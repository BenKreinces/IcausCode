from PySide6.QtCore import QPointF, QRectF, Qt
from PySide6.QtGui import QColor, QPainter, QPainterPath, QPen, QPolygonF
from PySide6.QtWidgets import QGraphicsObject


class EngineItem(QGraphicsObject):
    def __init__(self, parent=None):
        super().__init__(parent)

    def boundingRect(self) -> QRectF:
        return QRectF(-70, -110, 140, 250)

    def paint(self, painter: QPainter, option, widget=None):
        painter.setRenderHint(QPainter.Antialiasing)
        pen = QPen(QColor(20, 20, 20), 3)
        painter.setPen(pen)
        painter.setBrush(Qt.NoBrush)

        left_top = QPolygonF([QPointF(-46, -96), QPointF(-10, -96), QPointF(-28, -58)])
        right_top = QPolygonF([QPointF(10, -96), QPointF(46, -96), QPointF(28, -58)])
        painter.drawPolygon(left_top)
        painter.drawPolygon(right_top)
        painter.drawRect(QRectF(-50, -58, 100, 34))

        path = QPainterPath()
        path.moveTo(-50, -24)
        path.lineTo(-50, 74)
        path.cubicTo(-50, 96, -30, 110, -24, 130)
        path.cubicTo(-20, 146, -24, 158, -30, 170)
        path.cubicTo(-14, 176, 14, 176, 30, 170)
        path.cubicTo(24, 158, 20, 146, 24, 130)
        path.cubicTo(30, 110, 50, 96, 50, 74)
        path.lineTo(50, -24)
        painter.drawPath(path)

        painter.drawLine(-36, -24, 0, 44)
        painter.drawLine(36, -24, 0, 44)
        inner_v = QPolygonF([QPointF(-18, -24), QPointF(18, -24), QPointF(0, 10)])
        painter.drawPolygon(inner_v)
