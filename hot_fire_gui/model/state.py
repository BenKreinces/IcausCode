from dataclasses import dataclass


@dataclass
class StandState:
    lng_vent_open: bool = False
    lng_press_open: bool = False
    lng_fill_open: bool = False
    lng_dump_open: bool = False
    lng_run_open: bool = False

    lox_vent_open: bool = False
    lox_press_open: bool = False
    lox_fill_open: bool = False
    lox_dump_open: bool = False
    lox_run_open: bool = False

    lng_tank_level: float = 0.25
    lox_tank_level: float = 0.25

    lng_tank_pressure_psi: float = 120.0
    lox_tank_pressure_psi: float = 125.0
    lng_line_pressure_psi: float = 80.0
    lox_line_pressure_psi: float = 82.0

    lng_pump_rpm: float = 0.0
    lng_pump_power_kw: float = 0.0
    lng_pump_voltage_v: float = 0.0
    lng_pump_current_a: float = 0.0

    lox_pump_rpm: float = 0.0
    lox_pump_power_kw: float = 0.0
    lox_pump_voltage_v: float = 0.0
    lox_pump_current_a: float = 0.0

    mode_text: str = "SAFE"
