import time
import pyqtgraph as pg

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QComboBox,
    QPushButton,
    QColorDialog,
)

from config.plot_fields import PLOT_FIELDS


class LiveDataPlotterWindow(QMainWindow):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window

        self.selected_fields = []
        self.lines = {}
        self.y_data = {}
        self.x_data = []
        self.start_time = time.time()

        self.colors = [
            "red", "green", "blue", "cyan", "magenta",
            "yellow", "white", "orange", "purple"
        ]
        self.field_colors = {}

        self.setWindowTitle("Live Data Plotter")
        self.resize(1000, 600)

        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)

        controls = QHBoxLayout()
        layout.addLayout(controls)

        self.field_selector = QComboBox()
        self.field_selector.addItems(sorted(PLOT_FIELDS.keys()))
        controls.addWidget(self.field_selector)

        self.add_plot_btn = QPushButton("Add Plot")
        self.add_plot_btn.clicked.connect(self.add_plot)
        controls.addWidget(self.add_plot_btn)

        self.remove_plot_btn = QPushButton("Remove Plot")
        self.remove_plot_btn.clicked.connect(self.remove_plot)
        controls.addWidget(self.remove_plot_btn)

        self.change_color_btn = QPushButton("Change Color")
        self.change_color_btn.clicked.connect(self.change_color)
        controls.addWidget(self.change_color_btn)

        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setLabel("bottom", "Time", "s")
        self.plot_widget.addLegend()
        layout.addWidget(self.plot_widget)

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(100)

    def add_plot(self):
        field = self.field_selector.currentText()
        if not field or field in self.selected_fields:
            return

        self.selected_fields.append(field)
        self.y_data[field] = []

        if field not in self.field_colors:
            self.field_colors[field] = self.colors[len(self.field_colors) % len(self.colors)]

        color = self.field_colors[field]
        self.lines[field] = self.plot_widget.plot([], [], pen=color, name=field)

    def remove_plot(self):
        field = self.field_selector.currentText()
        if field not in self.selected_fields:
            return

        self.selected_fields.remove(field)
        if field in self.lines:
            self.plot_widget.removeItem(self.lines[field])
            del self.lines[field]
        if field in self.y_data:
            del self.y_data[field]

    def change_color(self):
        field = self.field_selector.currentText()
        if field not in self.selected_fields:
            return

        color = QColorDialog.getColor()
        if color.isValid():
            self.field_colors[field] = color.name()
            self.lines[field].setPen(color.name())

    def update_plot(self):
        now = time.time() - self.start_time
        self.x_data.append(now)

        state = self.main_window.state

        for field in self.selected_fields:
            value = PLOT_FIELDS[field](state)
            self.y_data[field].append(value)

            x_vals = self.x_data[-len(self.y_data[field]):]
            self.lines[field].setData(x_vals, self.y_data[field])

    def closeEvent(self, event):
        self.timer.stop()
        super().closeEvent(event)