from PySide6.QtCore import Property, QRectF, Qt, QPropertyAnimation
from PySide6.QtGui import QBrush, QColor, QFont, QPainter, QPainterPath, QPen
from PySide6.QtWidgets import QGraphicsObject, QGraphicsSimpleTextItem


class TankItem(QGraphicsObject):
    def __init__(self, width=100, height=180, tag="T-1", parent=None):
        super().__init__(parent)
        self.width = width
        self.height = height
        self.tag = tag
        self._level = 0.0
        self.label_item = QGraphicsSimpleTextItem(self.tag, self)
        self.label_item.setFont(QFont("Arial", 11, QFont.Bold))
        self.label_item.setPos(self.width + 10, 80)
        self.percent_item = QGraphicsSimpleTextItem("0%", self)
        self.percent_item.setFont(QFont("Consolas", 11))
        self.percent_item.setPos(self.width + 10, 100)

    def boundingRect(self) -> QRectF:
        return QRectF(-2, -2, self.width + 120, self.height + 56)

    def paint(self, painter: QPainter, option, widget=None):
        painter.setRenderHint(QPainter.Antialiasing)
        outline_rect = QRectF(0, 0, self.width, self.height)
        painter.setPen(QPen(QColor(20, 20, 20), 3))
        painter.setBrush(QBrush(QColor(248, 248, 248)))
        painter.drawRoundedRect(outline_rect, 18, 18)
        inner_rect = outline_rect.adjusted(8, 8, -8, -8)
        fill_height = inner_rect.height() * self._level
        fill_rect = QRectF(inner_rect.left(), inner_rect.bottom() - fill_height, inner_rect.width(), fill_height)
        painter.setPen(Qt.NoPen)
        painter.setBrush(QBrush(QColor(70, 140, 220, 180)))
        clip_path = QPainterPath()
        clip_path.addRoundedRect(inner_rect, 12, 12)
        painter.save()
        painter.setClipPath(clip_path)
        painter.drawRect(fill_rect)
        if fill_height > 2:
            painter.setPen(QPen(QColor(50, 100, 180), 2))
            painter.drawLine(fill_rect.left(), fill_rect.top(), fill_rect.right(), fill_rect.top())
        painter.restore()

    def get_level(self) -> float:
        return self._level

    def set_level(self, level: float):
        self._level = max(0.0, min(1.0, level))
        self.percent_item.setText(f"{self._level * 100:0.0f}%")
        self.update()

    level = Property(float, get_level, set_level)

    def animate_to_level(self, level: float, duration_ms: int = 800):
        anim = QPropertyAnimation(self, b"level")
        anim.setDuration(duration_ms)
        anim.setStartValue(self._level)
        anim.setEndValue(max(0.0, min(1.0, level)))
        anim.start()
        self._anim = anim
