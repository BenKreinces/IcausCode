from PySide6.QtGui import QBrush, QColor

from control.simulator import StandSimulator
from model.state import StandState


class StandController:
    BUTTON_LABELS = {
        "lng_vent": "LNG VENT",
        "lng_press": "LNG PRESS",
        "lng_fill": "LNG FILL",
        "lng_dump": "LNG DUMP",
        "lng_run": "LNG RUN",
        "lox_vent": "LOX VENT",
        "lox_press": "LOX PRESS",
        "lox_fill": "LOX FILL",
        "lox_dump": "LOX DUMP",
        "lox_run": "LOX RUN",
    }

    def __init__(self, state: StandState, view, side_panel):
        self.state = state
        self.view = view
        self.side_panel = side_panel
        self.simulator = StandSimulator()

    def toggle_valve(self, valve_id: str):
        attr = f"{valve_id}_open"
        is_open = not getattr(self.state, attr)
        setattr(self.state, attr, is_open)
        self.view.valves[valve_id].set_open(is_open)
        self.update_mode_text()
        self.apply_state_to_scene()

    def update_mode_text(self):
        open_count = sum(getattr(self.state, f"{k}_open") for k in self.BUTTON_LABELS)
        self.state.mode_text = "SAFE" if open_count == 0 else f"ACTIVE ({open_count} valve{'s' if open_count != 1 else ''} open)"

    def simulation_step(self):
        self.simulator.step(self.state, self.view)
        self.update_mode_text()
        self.apply_state_to_scene()

    def apply_state_to_scene(self):
        self.side_panel.mode_label.setText(f"System state: {self.state.mode_text}")
        self.view.lng_tank_pt.set_value(f"{self.state.lng_tank_pressure_psi:0.1f} psi")
        self.view.lox_tank_pt.set_value(f"{self.state.lox_tank_pressure_psi:0.1f} psi")
        self.view.lng_line_pt.set_value(f"{self.state.lng_line_pressure_psi:0.1f} psi")
        self.view.lox_line_pt.set_value(f"{self.state.lox_line_pressure_psi:0.1f} psi")
        lng_flow = max(0.0, (self.state.lng_line_pressure_psi / 80.0) * (1.0 if self.state.lng_run_open else 0.0))
        lox_flow = max(0.0, (self.state.lox_line_pressure_psi / 80.0) * (1.0 if self.state.lox_run_open else 0.0))
        self.view.lng_flow_ft.set_value(f"{lng_flow:0.2f} kg/s")
        self.view.lox_flow_ft.set_value(f"{lox_flow:0.2f} kg/s")

        for valve_id, label in self.BUTTON_LABELS.items():
            is_open = getattr(self.state, f"{valve_id}_open")
            self.view.valves[valve_id].state_item.setText("OPEN" if is_open else "CLOSED")
            self.view.valves[valve_id].state_item.setBrush(QBrush(QColor(20, 120, 20) if is_open else QColor(150, 20, 20)))
            self.side_panel.valve_buttons[valve_id].setText(f"{label}: {'OPEN' if is_open else 'CLOSED'}")

        self.side_panel.state_display.setText(
            "Mode          : {mode}\n"
            "\n"
            "Pressures\n"
            "LNG Tank PT   : {lng_pt:6.1f} psi\n"
            "LNG Line PT   : {lng_line:6.1f} psi\n"
            "LOX Tank PT   : {lox_pt:6.1f} psi\n"
            "LOX Line PT   : {lox_line:6.1f} psi\n"
            "\n"
            "Flows\n"
            "LNG Flow FT   : {lng_flow:6.2f} kg/s\n"
            "LOX Flow FT   : {lox_flow:6.2f} kg/s\n"
            "\n"
            "Tank Levels\n"
            "LNG Tank      : {lng_lvl:5.1f}%\n"
            "LOX Tank      : {lox_lvl:5.1f}%\n"
            "\n"
            "Pump Data\n"
            "LNG Pump RPM  : {lng_rpm:7.0f} rpm\n"
            "LNG Pump Pwr  : {lng_pwr:7.2f} kW\n"
            "LNG Pump V    : {lng_volt:7.2f} V\n"
            "LNG Pump I    : {lng_curr:7.2f} A\n"
            "LOX Pump RPM  : {lox_rpm:7.0f} rpm\n"
            "LOX Pump Pwr  : {lox_pwr:7.2f} kW\n"
            "LOX Pump V    : {lox_volt:7.2f} V\n"
            "LOX Pump I    : {lox_curr:7.2f} A\n"
            "\n"
            "Valves\n"
            "LNG VENT      : {lng_vent}\n"
            "LNG PRESS     : {lng_press}\n"
            "LNG FILL      : {lng_fill}\n"
            "LNG DUMP      : {lng_dump}\n"
            "LNG RUN       : {lng_run}\n"
            "LOX VENT      : {lox_vent}\n"
            "LOX PRESS     : {lox_press}\n"
            "LOX FILL      : {lox_fill}\n"
            "LOX DUMP      : {lox_dump}\n"
            "LOX RUN       : {lox_run}".format(
                mode=self.state.mode_text,
                lng_pt=self.state.lng_tank_pressure_psi,
                lng_line=self.state.lng_line_pressure_psi,
                lox_pt=self.state.lox_tank_pressure_psi,
                lox_line=self.state.lox_line_pressure_psi,
                lng_flow=lng_flow,
                lox_flow=lox_flow,
                lng_lvl=self.state.lng_tank_level * 100,
                lox_lvl=self.state.lox_tank_level * 100,
                lng_rpm=self.state.lng_pump_rpm,
                lng_pwr=self.state.lng_pump_power_kw,
                lng_volt=self.state.lng_pump_voltage_v,
                lng_curr=self.state.lng_pump_current_a,
                lox_rpm=self.state.lox_pump_rpm,
                lox_pwr=self.state.lox_pump_power_kw,
                lox_volt=self.state.lox_pump_voltage_v,
                lox_curr=self.state.lox_pump_current_a,
                lng_vent="OPEN" if self.state.lng_vent_open else "CLOSED",
                lng_press="OPEN" if self.state.lng_press_open else "CLOSED",
                lng_fill="OPEN" if self.state.lng_fill_open else "CLOSED",
                lng_dump="OPEN" if self.state.lng_dump_open else "CLOSED",
                lng_run="OPEN" if self.state.lng_run_open else "CLOSED",
                lox_vent="OPEN" if self.state.lox_vent_open else "CLOSED",
                lox_press="OPEN" if self.state.lox_press_open else "CLOSED",
                lox_fill="OPEN" if self.state.lox_fill_open else "CLOSED",
                lox_dump="OPEN" if self.state.lox_dump_open else "CLOSED",
                lox_run="OPEN" if self.state.lox_run_open else "CLOSED",
            )
        )
