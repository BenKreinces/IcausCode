import random

from model.state import StandState


class StandSimulator:
    def step(self, state: StandState, view):
        if state.lng_run_open:
            state.lng_line_pressure_psi += (state.lng_tank_pressure_psi - state.lng_line_pressure_psi) * 0.18
            state.lng_tank_level = max(0.0, state.lng_tank_level - 0.01)
            view.lngtank.animate_to_level(state.lng_tank_level, 700)
        else:
            state.lng_line_pressure_psi *= 0.985

        if state.lox_run_open:
            state.lox_line_pressure_psi += (state.lox_tank_pressure_psi - state.lox_line_pressure_psi) * 0.18
            state.lox_tank_level = max(0.0, state.lox_tank_level - 0.01)
            view.loxtank.animate_to_level(state.lox_tank_level, 700)
        else:
            state.lox_line_pressure_psi *= 0.985

        state.lng_pump_rpm = 32000.0 if state.lng_run_open else 0.0
        state.lox_pump_rpm = 34000.0 if state.lox_run_open else 0.0
        state.lng_pump_voltage_v = 28.0 if state.lng_run_open else 0.0
        state.lox_pump_voltage_v = 28.0 if state.lox_run_open else 0.0
        state.lng_pump_current_a = 18.0 if state.lng_run_open else 0.0
        state.lox_pump_current_a = 20.0 if state.lox_run_open else 0.0
        state.lng_pump_power_kw = (state.lng_pump_voltage_v * state.lng_pump_current_a) / 1000.0
        state.lox_pump_power_kw = (state.lox_pump_voltage_v * state.lox_pump_current_a) / 1000.0

        if state.lng_press_open:
            state.lng_tank_pressure_psi += 2.0
        else:
            state.lng_tank_pressure_psi = 40 + 280 * state.lng_tank_level + random.uniform(-2.0, 2.0)
        if state.lox_press_open:
            state.lox_tank_pressure_psi += 2.0
        else:
            state.lox_tank_pressure_psi = 40 + 280 * state.lox_tank_level + random.uniform(-2.0, 2.0)
        if state.lng_vent_open:
            state.lng_tank_pressure_psi = max(0.0, state.lng_tank_pressure_psi - 4.0)
        if state.lox_vent_open:
            state.lox_tank_pressure_psi = max(0.0, state.lox_tank_pressure_psi - 4.0)

        if state.lng_fill_open:
            state.lng_tank_level = min(1.0, state.lng_tank_level + 0.02)
            view.lngtank.animate_to_level(state.lng_tank_level, 700)
        if state.lox_fill_open:
            state.lox_tank_level = min(1.0, state.lox_tank_level + 0.02)
            view.loxtank.animate_to_level(state.lox_tank_level, 700)
        if state.lng_dump_open:
            state.lng_tank_level = max(0.0, state.lng_tank_level - 0.02)
            view.lngtank.animate_to_level(state.lng_tank_level, 700)
        if state.lox_dump_open:
            state.lox_tank_level = max(0.0, state.lox_tank_level - 0.02)
            view.loxtank.animate_to_level(state.lox_tank_level, 700)
