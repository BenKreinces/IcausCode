from PySide6.QtCore import QPointF, Property, QRectF, Qt, QPropertyAnimation, Signal
from PySide6.QtGui import QBrush, QColor, QFont, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import QGraphicsObject, QGraphicsSimpleTextItem


class ValveItem(QGraphicsObject):
    clicked = Signal(str)

    def __init__(self, valve_id: str, label: str, parent=None):
        super().__init__(parent)
        self.valve_id = valve_id
        self.label = label
        self._angle = 0.0
        self._is_open = False
        self.status_text = "CLOSED"

        self.text_item = QGraphicsSimpleTextItem(self.label, self)
        self.text_item.setFont(QFont("Arial", 9, QFont.Bold))
        self.text_item.setPos(-32, 36)

        self.state_item = QGraphicsSimpleTextItem(self.status_text, self)
        self.state_item.setFont(QFont("Arial", 9))
        self.state_item.setBrush(QBrush(QColor(150, 20, 20)))
        self.state_item.setPos(-30, 52)

        self.setAcceptedMouseButtons(Qt.LeftButton)
        self.setCursor(Qt.PointingHandCursor)

    def boundingRect(self) -> QRectF:
        return QRectF(-44, -38, 92, 106)

    def paint(self, painter: QPainter, option, widget=None):
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setPen(QPen(QColor(20, 20, 20), 2))
        painter.setBrush(QBrush(QColor(248, 248, 248)))
        painter.drawLine(-40, 0, -28, 0)
        painter.drawLine(28, 0, 40, 0)

        left_triangle = QPolygonF([QPointF(18, 0), QPointF(-28, -20), QPointF(-28, 20)])
        right_triangle = QPolygonF([QPointF(-18, 0), QPointF(28, -20), QPointF(28, 20)])
        painter.drawPolygon(left_triangle)
        painter.drawPolygon(right_triangle)
        painter.drawEllipse(QRectF(-18, -18, 36, 36))

        painter.save()
        painter.rotate(self._angle)
        painter.setPen(QPen(QColor(20, 20, 20), 4))
        painter.drawLine(-12, 0, 12, 0)
        painter.restore()

    def get_angle(self) -> float:
        return self._angle

    def set_angle(self, angle: float):
        self._angle = angle
        self.update()

    angle = Property(float, get_angle, set_angle)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.clicked.emit(self.valve_id)
            event.accept()
            return
        super().mousePressEvent(event)

    def set_open(self, is_open: bool):
        self._is_open = is_open
        self.status_text = "OPEN" if is_open else "CLOSED"
        self.state_item.setText(self.status_text)
        self.state_item.setBrush(QBrush(QColor(20, 120, 20) if is_open else QColor(150, 20, 20)))
        anim = QPropertyAnimation(self, b"angle")
        anim.setDuration(125)
        anim.setStartValue(self._angle)
        anim.setEndValue(90.0 if is_open else 0.0)
        anim.start()
        self._anim = anim
