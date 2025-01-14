#include "../include/sycon/args.hpp"
#include "../include/sycon/constants.hpp"
#include "../include/sycon/externs.hpp"

#include <filesystem>
#include <string>

namespace sycon {

args_t args;

static std::string get_executable_name(char *argv[]) {
  std::filesystem::path exe_path = argv[0];
  return exe_path.filename();
}

void parse_args(int argc, char *argv[]) {
  std::string program = get_executable_name(argv);

  CLI::App app{program};

  app.add_option("--program", args.p4_prog_name, "P4 program name")->default_val(program);

  app.add_flag("--ucli", args.run_ucli, "Interactive user CLI")->default_val(DEFAULT_ARG_UCLI);

  app.add_flag("--wait-ports", args.wait_for_ports,
               "Wait for the ports to be up and running (only relevant when "
               "running with the ASIC, not with the model)")
      ->default_val(DEFAULT_WAIT_FOR_PORTS);

  app.add_flag("--model", args.model, "Run for the tofino model")->default_val(DEFAULT_RUN_WITH_MODEL);

  app.add_option("--tna", args.tna_version, "TNA version")->default_val(DEFAULT_TNA_VERSION);

  app.add_option("--expiration-time", args.expiration_time, "Expiration time in miliseconds (set to 0 to never expire)")
      ->default_val(DEFAULT_EXPIRATION_TIME);

  app.add_option("--in", args.in_port, "Input frontend port")->default_val(DEFAULT_IN_PORT);

  app.add_option("--out", args.out_port, "Output frontend port")->default_val(DEFAULT_OUT_PORT);

  nf_args(app);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    app.exit(e);
    exit(1);
  }
}

} // namespace sycon