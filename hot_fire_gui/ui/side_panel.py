from PySide6.QtCore import Qt
from PySide6.QtWidgets import QHBoxLayout, QLabel, QPushButton, QVBoxLayout, QWidget


class SidePanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedWidth(300)
        layout = QVBoxLayout(self)
        header = QLabel("Operator Panel")
        header.setStyleSheet("font-size: 18px; font-weight: bold;")
        layout.addWidget(header)
        self.mode_label = QLabel("System state: SAFE")
        self.mode_label.setWordWrap(True)
        layout.addWidget(self.mode_label)
        self.open_plotter_button = QPushButton("Open Plotter")
        self.open_plotter_button.setMinimumHeight(30)
        layout.addWidget(self.open_plotter_button)

        valves_title = QLabel("Valve Controls")
        valves_title.setStyleSheet("font-size: 15px; font-weight: bold; margin-top: 6px;")
        layout.addWidget(valves_title)
        valves_widget = QWidget()
        valves_layout = QHBoxLayout(valves_widget)
        valves_layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(valves_widget)
        lng_column, lox_column = QVBoxLayout(), QVBoxLayout()
        valves_layout.addLayout(lng_column)
        valves_layout.addLayout(lox_column)

        self.valve_buttons = {}
        for valve_id, text, column in [
            ("lng_vent", "LNG VENT", lng_column), ("lng_press", "LNG PRESS", lng_column),
            ("lng_fill", "LNG FILL", lng_column), ("lng_dump", "LNG DUMP", lng_column),
            ("lng_run", "LNG RUN", lng_column), ("lox_vent", "LOX VENT", lox_column),
            ("lox_press", "LOX PRESS", lox_column), ("lox_fill", "LOX FILL", lox_column),
            ("lox_dump", "LOX DUMP", lox_column), ("lox_run", "LOX RUN", lox_column),
        ]:
            btn = QPushButton(text)
            btn.setMinimumHeight(30)
            column.addWidget(btn)
            self.valve_buttons[valve_id] = btn

        state_title = QLabel("Stand Status")
        state_title.setStyleSheet("font-size: 15px; font-weight: bold; margin-top: 10px;")
        layout.addWidget(state_title)
        self.state_display = QLabel()
        self.state_display.setWordWrap(True)
        self.state_display.setAlignment(Qt.AlignTop | Qt.AlignLeft)
        self.state_display.setStyleSheet(
            "background: white; border: 1px solid rgb(190,190,190); padding: 8px; font-family: Consolas, monospace;"
        )
        layout.addWidget(self.state_display, stretch=1)
        self.mouse_pos_label = QLabel("Mouse: x=0.0, y=0.0")
        self.mouse_pos_label.setAlignment(Qt.AlignRight | Qt.AlignBottom)
        self.mouse_pos_label.setStyleSheet("font-size: 11px; color: rgb(90, 90, 90);")
        layout.addWidget(self.mouse_pos_label)
