from dataclasses import dataclass


@dataclass(frozen=True)
class Values:
    requested_bps: float
    pktgen_bps: float
    pktgen_pps: float
    dut_ingress_bps: float
    dut_ingress_pps: float
    dut_egress_bps: float
    dut_egress_pps: float
