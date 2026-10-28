from PySide6.QtCore import QTimer
from PySide6.QtWidgets import QHBoxLayout, QMainWindow, QWidget

from config.plot_fields import PLOT_FIELDS
from control.controller import StandController
from model.state import StandState
from ui.pid_view import PidView
from ui.plotter_window import LiveDataPlotterWindow
from ui.side_panel import SidePanel


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Rocket Test Stand GUI - P&ID Prototype")
        self.resize(1200, 720)

        central = QWidget()
        self.setCentralWidget(central)
        layout = QHBoxLayout(central)

        self.view = PidView()
        self.side_panel = SidePanel()
        layout.addWidget(self.view, stretch=1)
        layout.addWidget(self.side_panel)
        self.plotter_window = None

        self.controller = StandController(StandState(), self.view, self.side_panel)
        self.view.mouse_scene_changed.connect(self.update_mouse_position_label)
        self.side_panel.open_plotter_button.clicked.connect(self.open_plotter)
        for valve in self.view.valves.values():
            valve.clicked.connect(self.controller.toggle_valve)
        for valve_id, button in self.side_panel.valve_buttons.items():
            button.clicked.connect(lambda checked=False, vid=valve_id: self.controller.toggle_valve(vid))

        self.sim_timer = QTimer(self)
        self.sim_timer.timeout.connect(self._tick)
        self.sim_timer.start(10)
        self.controller.apply_state_to_scene()

    def update_mouse_position_label(self, x: float, y: float):
        self.side_panel.mouse_pos_label.setText(f"Mouse: x={x:0.1f}, y={y:0.1f}")

    def open_plotter(self):
        if self.plotter_window is None:
            self.plotter_window = LiveDataPlotterWindow(self)
            self.plotter_window.destroyed.connect(self._on_plotter_closed)
        self.plotter_window.show()
        self.plotter_window.raise_()
        self.plotter_window.activateWindow()

    def _on_plotter_closed(self):
        self.plotter_window = None

    def _tick(self):
        self.controller.simulation_step()

    def get_plot_data(self):
        state = self.controller.state
        return {name: getter(state) for name, getter in PLOT_FIELDS.items()}
