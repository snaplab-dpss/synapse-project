import re
import select
import threading
from collections import deque
import sys
import termios
import time
import tty
import socket
import paramiko
import os

from pathlib import Path
from typing import TextIO, Union, Optional, Callable

from .command import Command


class RemoteCommand(Command):
    # A tail of the background reader's output is kept for stop_output_reader_thread to return;
    # the log file always gets all of it.
    MAX_BACKGROUND_OUTPUT = 8 * 1024 * 1024
    OUTPUT_READER_JOIN_TIMEOUT_SEC = 5
    OUTPUT_READER_CHUNK = 64 * 1024

    def __init__(
        self,
        ssh_client: paramiko.SSHClient,
        command: str,
        dir: Optional[Union[str, Path]] = None,
        source_bashrc: bool = True,
        log_file: Optional[TextIO] = None,
        pty: bool = False,
    ) -> None:
        super().__init__(command, dir, source_bashrc, log_file)
        self.ssh_client = ssh_client

        transport = ssh_client.get_transport()

        if transport is None:
            raise RuntimeError("Failed to get transport from client.")

        session = transport.open_session()

        if pty:
            session.setblocking(0)
            session.get_pty()

        session.exec_command(self.command)
        self.cmd_ = session

        # Set by spawn_output_reader_thread. See the comment there for why this exists.
        self.output_reader_thread: Optional[threading.Thread] = None
        self.output_reader_stop = threading.Event()
        self.output_reader_lock = threading.Lock()
        self.output_chunks: deque = deque()
        self.output_chunks_len = 0
        self.output_discarded_in_background = 0

    def spawn_output_reader_thread(self) -> None:
        """Start a background thread that keeps reading this command's output, and RETURN AT ONCE.

        Nothing reads a command's output except `watch` and `run_console_commands`, so between two
        of those calls the output just sits in the SSH channel. Once it fills the channel's window
        the REMOTE process blocks in write(), which for a controller means it stops answering
        packets mid-experiment -- and meanwhile none of what it printed reaches its log file. A
        chatty controller therefore both stalls and goes unrecorded, silently.

        This thread is the only reader while it lives, so `watch` refuses to run alongside it and
        `run_console_commands` pauses it. Stop it with stop_output_reader_thread.
        """
        if self.output_reader_thread is not None:
            return

        self.output_reader_stop.clear()
        self.output_reader_thread = threading.Thread(target=self._read_output_until_stopped, daemon=True)
        self.output_reader_thread.start()

    def stop_output_reader_thread(self) -> str:
        """Stop the background reader and return what it read (see MAX_BACKGROUND_OUTPUT)."""
        if self.output_reader_thread is None:
            return ""

        self.output_reader_stop.set()
        self.output_reader_thread.join(timeout=self.OUTPUT_READER_JOIN_TIMEOUT_SEC)
        self.output_reader_thread = None

        with self.output_reader_lock:
            output = "".join(self.output_chunks)
            discarded = self.output_discarded_in_background
            self.output_chunks.clear()
            self.output_chunks_len = 0
            self.output_discarded_in_background = 0

        if discarded:
            print(f"warning: dropped {discarded} bytes of background output (the log file has them all)")

        return output

    def _reading_output_in_background(self) -> bool:
        return self.output_reader_thread is not None

    def _read_output_until_stopped(self) -> None:
        # Kept as chunks rather than one string: appending to a multi-megabyte string copies it
        # every time, which made the reader slower than the controller it is supposed to keep up
        # with -- and a reader that cannot keep up is the very problem this thread exists to avoid.
        while not self.output_reader_stop.is_set():
            read_something = False

            for ready, receive in ((self.cmd_.recv_ready, self.cmd_.recv), (self.cmd_.recv_stderr_ready, self.cmd_.recv_stderr)):
                while ready():
                    read_something = True
                    decoded_data = receive(self.OUTPUT_READER_CHUNK).decode("utf-8", errors="replace")

                    with self.output_reader_lock:
                        # The log file always gets everything; memory keeps only a bounded tail.
                        if self.log_file:
                            self.log_file.write(decoded_data)
                            self.log_file.flush()

                        self.output_chunks.append(decoded_data)
                        self.output_chunks_len += len(decoded_data)

                        while self.output_chunks_len - len(self.output_chunks[0]) >= self.MAX_BACKGROUND_OUTPUT:
                            dropped = self.output_chunks.popleft()
                            self.output_chunks_len -= len(dropped)
                            self.output_discarded_in_background += len(dropped)

            if not read_something:
                time.sleep(0.01)

    def send(self, data: Union[str, bytes]) -> None:
        if isinstance(data, str):
            data = data.encode("utf-8")
        self.cmd_.send(data)

    def recv(self, size: int) -> str:
        data = self.cmd_.recv(size)
        return data.decode("utf-8")

    def recv_stderr(self, size: int) -> str:
        data = self.cmd_.recv_stderr(size)
        return data.decode("utf-8")

    def exit_status_ready(self) -> bool:
        return self.cmd_.exit_status_ready()

    def flush(
        self,
        keyboard_int: Optional[Callable[[], None]] = None,
    ):
        output = ""

        keep_going = True
        try:
            while keep_going:
                time.sleep(0.1)

                # Consume everything that comes out.
                keep_going = len(output) > 0

                while self.cmd_.recv_ready():
                    data = self.cmd_.recv(512)
                    decoded_data = data.decode("utf-8")
                    output += decoded_data
                    if self.log_file:
                        self.log_file.write(decoded_data)
                        self.log_file.flush()

                while self.cmd_.recv_stderr_ready():
                    data = self.cmd_.recv_stderr(512)
                    decoded_data = data.decode("utf-8")
                    output += decoded_data
                    if self.log_file:
                        self.log_file.write(decoded_data)
                        self.log_file.flush()

        except KeyboardInterrupt:
            if keyboard_int is not None:
                keyboard_int()
            raise

    def watch(
        self,
        stop_condition: Optional[Callable[[], bool]] = None,
        keyboard_int: Optional[Callable[[], None]] = None,
        timeout: Optional[float] = None,
        stop_pattern: Optional[str] = None,
        max_match_length: Optional[int] = None,
    ) -> str:
        # The background reader would consume the very output being waited for here, and whether a
        # pattern arrived before or after this call became unanswerable. Refuse instead of racing
        # it: the caller has to decide, and a silently missed pattern is the worst outcome.
        if self._reading_output_in_background():
            raise RuntimeError("cannot watch() while the background output reader is running; stop_output_reader_thread() first")

        if stop_condition is None:
            stop_condition = self.cmd_.exit_status_ready

        assert stop_condition is not None

        if timeout is None:
            deadline = None
        else:
            deadline = time.time() + timeout

        if max_match_length is None:
            max_match_length = 1024

        output = ""
        searched_upto = 0

        def continue_running():
            nonlocal searched_upto

            if (deadline is not None) and (time.time() > deadline):
                return False

            if stop_pattern is not None:
                # Search everything that arrived since the last check, plus an overlap so a match
                # straddling the boundary is still found. Looking only at the last max_match_length
                # characters instead would skip PAST the pattern whenever more than that arrives
                # between two checks: the debug controller prints its prompt and then keeps logging,
                # which left the prompt thousands of characters from the end and hung the wait.
                start = max(0, searched_upto - max_match_length)
                if re.search(stop_pattern, output[start:]):
                    return False
                searched_upto = len(output)

            return not stop_condition()

        keep_going = True
        try:
            while keep_going:
                time.sleep(0.01)

                # We consume the output one more time after it's done.
                # This prevents us from missing the last bytes.
                keep_going = continue_running()

                while self.cmd_.recv_ready():
                    data = self.cmd_.recv(512)
                    decoded_data = data.decode("utf-8")
                    output += decoded_data
                    if self.log_file:
                        self.log_file.write(decoded_data)
                        self.log_file.flush()

                while self.cmd_.recv_stderr_ready():
                    data = self.cmd_.recv_stderr(512)
                    decoded_data = data.decode("utf-8")
                    output += decoded_data
                    if self.log_file:
                        self.log_file.write(decoded_data)
                        self.log_file.flush()

        except KeyboardInterrupt:
            if keyboard_int is not None:
                keyboard_int()
            raise

        return output

    def recv_exit_status(self) -> int:
        return self.cmd_.recv_exit_status()

    def fileno(self) -> int:
        return self.cmd_.fileno()

    def run_console_commands(
        self,
        commands: Union[str, list[str]],
        timeout: Optional[float] = None,
        console_pattern: Optional[str] = None,
    ) -> str:
        if not isinstance(commands, list):
            commands = [commands]

        if console_pattern is not None:
            console_pattern_len = len(console_pattern)
        else:
            console_pattern_len = None

        # Unlike watch(), this is unambiguous: the command is sent only after the reader has
        # stopped, so its answer can only arrive on the channel, never into the reader's buffer.
        # Pause rather than refuse, so that callers do not have to know the reader exists.
        was_reading_in_background = self._reading_output_in_background()
        if was_reading_in_background:
            self.stop_output_reader_thread()

        output = ""
        try:
            for cmd in commands:
                self.send(cmd + "\n")
                output += self.watch(
                    keyboard_int=lambda: self.send("\x03"),
                    timeout=timeout,
                    stop_pattern=console_pattern,
                    max_match_length=console_pattern_len,
                )
        finally:
            if was_reading_in_background:
                self.spawn_output_reader_thread()

        return output

    def posix_shell(self) -> None:
        oldtty = termios.tcgetattr(sys.stdin)
        stdin_blocking = os.get_blocking(sys.stdin.fileno())
        try:
            tty.setcbreak(sys.stdin.fileno())
            os.set_blocking(sys.stdin.fileno(), False)

            self.send("\n")
            print("\n")

            while True:
                r, _, _ = select.select([sys.stdout, sys.stderr, sys.stdin], [], [])
                if sys.stdout in r:
                    try:
                        data = self.recv(512)
                        if len(data) == 0:
                            break

                        sys.stdout.write(data)
                        sys.stdout.flush()

                    except socket.timeout:
                        pass

                if sys.stderr in r:
                    try:
                        data = self.recv_stderr(512)
                        sys.stderr.write(data)
                        sys.stderr.flush()

                    except socket.timeout:
                        pass

                if sys.stdin in r:
                    x = sys.stdin.read(512)
                    if len(x) == 0:
                        break

                    if x.isprintable() or x in ["\r", "\n", "\t", "\x7f"]:
                        # If backspace, delete the last character
                        if x == "\x7f":
                            sys.stdout.write("\b \b")
                        else:
                            sys.stdout.write(x)
                        sys.stdout.flush()
                        self.send(x)

        finally:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, oldtty)
            os.set_blocking(sys.stdin.fileno(), stdin_blocking)

    def __del__(self):
        if hasattr(self, "cmd_"):
            self.cmd_.close()
