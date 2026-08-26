from PySide6.QtCore import Signal
from PySide6.QtGui import QBrush, QColor, QPainter
from PySide6.QtWidgets import QGraphicsScene, QGraphicsView

from ui.items.engine_item import EngineItem
from ui.items.instrument_label_item import InstrumentLabelItem
from ui.items.pipe_item import PipeItem
from ui.items.pump_item import PumpItem
from ui.items.tank_item import TankItem
from ui.items.valve_item import ValveItem


class PidView(QGraphicsView):
    mouse_scene_changed = Signal(float, float)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setRenderHints(QPainter.Antialiasing | QPainter.TextAntialiasing)
        self.setScene(QGraphicsScene(self))
        self.setBackgroundBrush(QBrush(QColor(235, 235, 235)))
        self.setSceneRect(0, 0, 900, 540)
        self.setDragMode(QGraphicsView.ScrollHandDrag)
        self.setViewportUpdateMode(QGraphicsView.BoundingRectViewportUpdate)
        self.setMouseTracking(True)
        self.valves = {}
        self._build_scene()

    def _add_valve(self, valve_id: str, label: str, x: float, y: float):
        valve = ValveItem(valve_id=valve_id, label=label)
        valve.setPos(x, y)
        self.scene().addItem(valve)
        self.valves[valve_id] = valve
        return valve

    def _build_scene(self):
        s = self.scene()
        self.loxtank = TankItem(tag="LOX TANK")
        self.loxtank.setPos(670, 40)
        self.loxtank.set_level(0.25)
        s.addItem(self.loxtank)
        self.lngtank = TankItem(tag="LNG TANK")
        self.lngtank.setPos(120, 40)
        self.lngtank.set_level(0.25)
        s.addItem(self.lngtank)

        for x1, y1, x2, y2 in [
            (720, 40, 720, 10), (720, 10, 820, 10), (720, 10, 620, 10), (720, 220, 720, 400),
            (720, 300, 820, 300), (720, 400, 820, 400), (720, 300, 560, 300), (560, 300, 560, 212),
            (560, 168, 560, 130), (320, 300, 320, 378), (320, 422, 320, 460), (170, 40, 170, 10),
            (170, 10, 60, 10), (170, 10, 270, 10), (170, 220, 170, 400), (170, 300, 60, 300),
            (170, 400, 60, 400), (170, 300, 320, 300), (270, 300, 320, 300), (620, 300, 560, 300),
        ]:
            s.addItem(PipeItem(x1, y1, x2, y2))

        for valve_id, label, x, y in [
            ("lox_vent", "LOX VENT", 820, 10), ("lox_press", "LOX PRESS", 620, 10),
            ("lox_fill", "LOX FILL", 820, 300), ("lox_dump", "LOX DUMP", 820, 400),
            ("lox_run", "LOX RUN", 620, 300), ("lng_vent", "LNG VENT", 60, 10),
            ("lng_press", "LNG PRESS", 270, 10), ("lng_fill", "LNG FILL", 60, 300),
            ("lng_dump", "LNG DUMP", 60, 400), ("lng_run", "LNG RUN", 270, 300),
        ]:
            self._add_valve(valve_id, label, x, y)

        self.lng_pump = PumpItem(label="LNG PUMP", point_up=False)
        self.lng_pump.setPos(320, 400)
        s.addItem(self.lng_pump)
        self.lox_pump = PumpItem(label="LOX PUMP", point_up=True)
        self.lox_pump.setPos(560, 190)
        s.addItem(self.lox_pump)

        self.lng_tank_pt = InstrumentLabelItem("PT-LNG", "120.0 psi")
        self.lng_tank_pt.setPos(130, -40)
        s.addItem(self.lng_tank_pt)
        self.lox_tank_pt = InstrumentLabelItem("PT-LOX", "125.0 psi")
        self.lox_tank_pt.setPos(700, -40)
        s.addItem(self.lox_tank_pt)
        self.lng_line_pt = InstrumentLabelItem("PT-LINE-LNG", "80.0 psi")
        self.lng_line_pt.setPos(240, 230)
        s.addItem(self.lng_line_pt)
        self.lox_line_pt = InstrumentLabelItem("PT-LINE-LOX", "82.0 psi")
        self.lox_line_pt.setPos(580, 380)
        s.addItem(self.lox_line_pt)
        self.lng_flow_ft = InstrumentLabelItem("FT-LNG", "0.00 kg/s")
        self.lng_flow_ft.setPos(240, 180)
        s.addItem(self.lng_flow_ft)
        self.lox_flow_ft = InstrumentLabelItem("FT-LOX", "0.00 kg/s")
        self.lox_flow_ft.setPos(580, 430)
        s.addItem(self.lox_flow_ft)
        self.engine = EngineItem()
        self.engine.setPos(440, 302)
        s.addItem(self.engine)

    def mouseMoveEvent(self, event):
        scene_pos = self.mapToScene(event.position().toPoint())
        self.mouse_scene_changed.emit(scene_pos.x(), scene_pos.y())
        super().mouseMoveEvent(event)
