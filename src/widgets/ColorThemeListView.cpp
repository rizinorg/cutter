#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QPainter>
#include <QFontMetrics>
#include <QScreen>
#include <QJsonArray>
#include <QScrollBar>
#include <QApplication>
#include <QSvgRenderer>
#include <QMouseEvent>

#include "common/Configuration.h"
#include "common/ColorThemeWorker.h"

#include "widgets/ColorThemeListView.h"


struct OptionInfo {
    QString info;
    QString displayingtext;
};

extern const QMap<QString, OptionInfo> optionInfoMap__;

ColorOptionDelegate::ColorOptionDelegate(QObject* parent) : QStyledItemDelegate (parent)
{
    resetButtonPixmap = getPixmapFromSvg(":/img/icons/reset.svg", qApp->palette().text().color());
    connect(qApp, &QGuiApplication::paletteChanged, this,
            [this](){
        resetButtonPixmap = getPixmapFromSvg(":/img/icons/reset.svg", qApp->palette().text().color());
    });
}

void ColorOptionDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    int margin = this->margin * painter->device()->devicePixelRatioF();
    painter->save();
    painter->setFont(option.font);
    painter->setRenderHint(QPainter::Antialiasing);

    ColorOption currCO = index.data(Qt::UserRole).value<ColorOption>();

    int penWidth = painter->pen().width();
    int fontHeight = painter->fontMetrics().height();
    QPoint tl = option.rect.topLeft();

    QRect optionNameRect;
    optionNameRect.setTopLeft(tl + QPoint(margin, penWidth));
    optionNameRect.setWidth(option.rect.width() - margin * 2);
    optionNameRect.setHeight(fontHeight);

    QRect optionRect;
    optionRect.setTopLeft(optionNameRect.bottomLeft() + QPoint(margin / 2, margin / 2));
    optionRect.setWidth(option.rect.width() - (optionRect.topLeft() - tl).x() * 2);
    optionRect.setHeight(option.rect.height() - (optionRect.topLeft() - tl).y() - margin);

    QRect colorRect;
    colorRect.setTopLeft(optionRect.topLeft() + QPoint(margin / 4, margin / 4));
    colorRect.setBottom(optionRect.bottom() - margin / 4);
    colorRect.setWidth(colorRect.height());

    QRect descTextRect;
    descTextRect.setTopLeft(colorRect.topRight() + QPoint(margin, colorRect.height() / 2 - fontHeight / 2));
    descTextRect.setWidth(optionRect.width() - (descTextRect.left() - optionRect.left()) - margin);
    descTextRect.setHeight(fontHeight);

    bool paintResetButton = false;
    QRect resetButtonRect;

    if (option.state & (QStyle::State_Selected | QStyle::State_MouseOver)) {
        QBrush br;
        QPen pen;
        if (option.state.testFlag(QStyle::State_Selected)) {
            QColor c = qApp->palette().highlight().color();
            c.setAlphaF(0.4);
            br = c;
            pen = QPen(qApp->palette().highlight().color(), margin / 2);
            if (currCO.changed) {
                paintResetButton = true;
                descTextRect.setWidth(descTextRect.width() - descTextRect.height() - margin / 2);
                resetButtonRect.setTopLeft(descTextRect.topRight() + QPoint(margin, 0));
                resetButtonRect.setWidth(descTextRect.height());
                resetButtonRect.setHeight(descTextRect.height());
                resetButtonRect.setSize(resetButtonRect.size() * 1.0);
            }
        } else {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
            QColor placeholderColor = qApp->palette().placeholderText().color();
#else
            QColor placeholderColor = qApp->palette().text().color();
            placeholderColor.setAlphaF(0.5);
#endif
            QColor c = placeholderColor;
            c.setAlphaF(0.2);
            br = c;
            pen = QPen(placeholderColor.darker(), margin / 2);
        }

        painter->fillRect(option.rect, br);

        painter->setPen(pen);
        int pw = painter->pen().width() / 2;
        QPoint top = option.rect.topLeft() + QPoint(pw, pw);
        QPoint bottom = option.rect.bottomLeft() - QPoint(-pw, pw - 1);
        painter->drawLine(top, bottom);
    }

    if (paintResetButton) {
        painter->drawPixmap(resetButtonRect, resetButtonPixmap);
        auto self = const_cast<ColorOptionDelegate*>(this);
        self->resetButtonRect = resetButtonRect;
    }
    if (option.rect.contains(this->resetButtonRect) && this->resetButtonRect != resetButtonRect) {
        auto self = const_cast<ColorOptionDelegate*>(this);
        self->resetButtonRect = QRect(0,0,0,0);
    }

    painter->setPen(qApp->palette().text().color());

    QString name = painter->fontMetrics().elidedText(
                       optionInfoMap__[currCO.optionName].displayingtext,
                       Qt::ElideRight, optionNameRect.width());
    painter->drawText(optionNameRect, name);

    QPainterPath roundedOptionRect;
    roundedOptionRect.addRoundedRect(optionRect, fontHeight / 4, fontHeight / 4);
    painter->setPen(qApp->palette().text().color());
    painter->drawPath(roundedOptionRect);

    QPainterPath roundedColorRect;
    roundedColorRect.addRoundedRect(colorRect, fontHeight / 4, fontHeight / 4);
    painter->setPen(Qt::NoPen);
    painter->fillPath(roundedColorRect, currCO.color);

    QString desc = painter->fontMetrics().elidedText(
                       currCO.optionName + ": " +
                       optionInfoMap__[currCO.optionName].info, Qt::ElideRight,
                       descTextRect.width());
    painter->setPen(qApp->palette().text().color());
    painter->setBrush(qApp->palette().text());
    painter->drawText(descTextRect, desc);

    painter->restore();
}

QSize ColorOptionDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    qreal margin = this->margin * option.widget->devicePixelRatioF();
    qreal fontHeight = option.fontMetrics.height();
    qreal h = QPen().width();
    h += fontHeight; // option name
    h += margin / 2; // margin between option rect and option name
    h += margin / 4; // margin betveen option rect and color rect
    h += fontHeight; // color rect
    h += margin / 4; // margin betveen option rect and color rect
    h += margin; // last margin

    Q_UNUSED(index)
    return QSize(-1, qRound(h));
}

QRect ColorOptionDelegate::getResetButtonRect() const
{
    return resetButtonRect;
}

QPixmap ColorOptionDelegate::getPixmapFromSvg(const QString& fileName, const QColor& after) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QPixmap();
    }
    QString data = file.readAll();
    data.replace(QRegExp("#[0-9a-fA-F]{6}"), QString("%1").arg(after.name()));

    QSvgRenderer svgRenderer(data.toUtf8());
    QPixmap pix(QSize(qApp->fontMetrics().height(), qApp->fontMetrics().height()));
    pix.fill(Qt::transparent);

    QPainter pixPainter(&pix);
    svgRenderer.render(&pixPainter);

    return pix;
}

ColorThemeListView::ColorThemeListView(QWidget *parent) :
    QListView (parent)
{
    setModel(new ColorSettingsModel(static_cast<QObject *>(this)));
    static_cast<ColorSettingsModel *>(this->model())->updateTheme();
    setItemDelegate(new ColorOptionDelegate(this));
    setResizeMode(ResizeMode::Adjust);

    QJsonArray rgb = qobject_cast<ColorSettingsModel*>(model())->getTheme()
                     .object().find("gui.background").value().toArray();
    if (rgb.size() == 3) {
        backgroundColor = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
    } else {
        backgroundColor = palette().base().color();
    }

    connect(&blinkTimer, &QTimer::timeout, this, &ColorThemeListView::blinkTimeout);

    blinkTimer.setInterval(400);
    blinkTimer.start();

    setMouseTracking(true);
}

void ColorThemeListView::currentChanged(const QModelIndex &current,
                                         const QModelIndex &previous)
{
    ColorOption prev = previous.data(Qt::UserRole).value<ColorOption>();
    Config()->setColor(prev.optionName, prev.color);
    if (ThemeWorker().radare2SpecificOptions.contains(prev.optionName)) {
        Core()->cmd(QString("ec %1 %2").arg(prev.optionName).arg(prev.color.name()));
    }

    QListView::currentChanged(current, previous);
    emit itemChanged(current.data(Qt::UserRole).value<ColorOption>().color);
}

void ColorThemeListView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                      const QVector<int>& roles)
{
    ColorOption curr = topLeft.data(Qt::UserRole).value<ColorOption>();
    if (curr.optionName == "gui.background") {
        backgroundColor = curr.color;
    }
    QListView::dataChanged(topLeft, bottomRight, roles);
    emit itemChanged(curr.color);
    emit dataChanged(curr);
}

void ColorThemeListView::mouseReleaseEvent(QMouseEvent* e)
{
    if (qobject_cast<ColorOptionDelegate*>(itemDelegate())->getResetButtonRect().contains(e->pos())) {
        auto model = qobject_cast<ColorSettingsModel*>(this->model());
        ColorOption co = currentIndex().data(Qt::UserRole).value<ColorOption>();
        co.changed = false;
        QJsonArray rgb = ThemeWorker().getTheme(
                             Config()->getColorTheme()).object()[co.optionName].toArray();
        co.color = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
        model->setData(currentIndex(), QVariant::fromValue(co));
        QCursor c;
        c.setShape(Qt::CursorShape::ArrowCursor);
        setCursor(c);
    }
}

void ColorThemeListView::mouseMoveEvent(QMouseEvent* e)
{
    if (qobject_cast<ColorOptionDelegate*>(itemDelegate())->getResetButtonRect().contains(e->pos())) {
        QCursor c;
        c.setShape(Qt::CursorShape::PointingHandCursor);
        setCursor(c);
    } else if (cursor().shape() == Qt::CursorShape::PointingHandCursor) {
        QCursor c;
        c.setShape(Qt::CursorShape::ArrowCursor);
        setCursor(c);
    }
}

void ColorThemeListView::blinkTimeout()
{
    static enum { Normal, Invisible } state = Normal;
    state = state == Normal ? Invisible : Normal;
    backgroundColor.setAlphaF(1);

    auto updateColor = [](const QString &name, const QColor &color) {
        Config()->setColor(name, color);
        if (ThemeWorker().radare2SpecificOptions.contains(name)) {
            Core()->cmd(QString("ec %1 %2").arg(name).arg(color.name()));
        }
    };

    ColorOption curr = currentIndex().data(Qt::UserRole).value<ColorOption>();
    switch (state) {
    case Normal:
        updateColor(curr.optionName, curr.color);
        break;
    case Invisible:
        updateColor(curr.optionName, backgroundColor);
        break;
    }
    emit blink();
}

ColorSettingsModel::ColorSettingsModel(QObject *parent) : QAbstractListModel (parent) { }

QVariant ColorSettingsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
      return QVariant();
    }

    if (index.row() < 0 || index.row() >= theme.size()) {
      return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(optionInfoMap__[theme.at(index.row()).optionName].displayingtext);
    }

    if (role == Qt::UserRole) {
        return QVariant::fromValue(theme.at(index.row()));
    }

    if (role == Qt::ToolTipRole) {
        return QVariant::fromValue(optionInfoMap__[theme.at(index.row()).optionName].info);
    }

    return QVariant();
}

bool ColorSettingsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole) {
        return false;
    }

    ColorOption currOpt = value.value<ColorOption>();
    theme[index.row()] = currOpt;
    emit dataChanged(index, index);
    return true;
}

void ColorSettingsModel::updateTheme()
{
    theme.clear();
    QJsonObject obj = ThemeWorker().getTheme(Config()->getColorTheme()).object();

    for (auto it = obj.constBegin(); it != obj.constEnd(); it++) {
        QJsonArray rgb = it.value().toArray();
        if (rgb.size() != 3) {
            continue;
        }
        theme.push_back({it.key(), QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()), false});
    }

    if (!theme.isEmpty()) {
        dataChanged(index(0), index(theme.size() - 1));
    }
}

QJsonDocument ColorSettingsModel::getTheme() const
{
    QJsonObject obj;
    int r, g, b;
    for (auto &it : theme) {
        it.color.getRgb(&r, &g, &b);
        obj.insert(it.optionName, QJsonArray({r, g, b}));
    }
    return QJsonDocument(obj);
}

const QMap<QString, OptionInfo> optionInfoMap__ = {
{
    "comment", {
        QObject::tr("Color of comment generated by radare2"),
        QObject::tr("Comment")
    }
},
{
    "usrcmt", {
        QObject::tr("Comment created by user"),
        QObject::tr("Color of user Comment")
    }
},
{
    "args", {
        "",
        "args"
    }
},
{
    "fname", {
        QObject::tr("Color of names of functions"),
        QObject::tr("Function name")
    }
},
{
    "floc", {
        QObject::tr("Color of function location"),
        QObject::tr("Function location")
    }
},
{
    "fline", {
        QObject::tr("Color of ascii line in left side that shows what opcodes are belong to function"),
        QObject::tr("Function line")
    }
},
{
    "flag", {
        QObject::tr("Color of flags (similar to bookmarks for offset)"),
        QObject::tr("Flag")
    }
},
{
    "label", {
        "",
        QObject::tr("Label")
    }
},
{
    "help", {
        "",
        QObject::tr("Help")
    }
},
{
    "flow", {
        QObject::tr("Color of lines showing jump destination"),
        QObject::tr("Flow")
    }
},
{
    "flow2", {
        "",
        QObject::tr("flow2")
    }
},
{
    "prompt", {
        QObject::tr("Info"),
        QObject::tr("prompt")
    }
},
{
    "offset", {
        QObject::tr("Color of offsets"),
        QObject::tr("Offset")
    }
},
{
    "input", {
        QObject::tr("Info"),
        QObject::tr("input")
    }
},
{
    "invalid", {
        QObject::tr("Invalid opcode color"),
        QObject::tr("invalid")
    }
},
{
    "other", {
        "",
        QObject::tr("other")
    }
},
{
    "b0x00", {
        QObject::tr("0x00 opcode color"),
        "b0x00"
    }
},
{
    "b0x7f", {
        QObject::tr("0x7f opcode color"),
        "b0x7f"
    }
},
{
    "b0xff", {
        QObject::tr("0xff opcode color"),
        "b0xff"
    }
},
{
    "math", {
        QObject::tr("Color of arithmetic opcodes (add, div, mul etc)"),
        QObject::tr("Arithmetic")
    }
},
{
    "bin", {
        QObject::tr("Color of binary operations (and, or, xor etc)."),
        QObject::tr("Binary")
    }
},
{
    "btext", {
        QObject::tr("Color of object names, commas between operators, squared brackets and operators "
            "inside them."),
        QObject::tr("Text")
    }
},
{
    "push", {
        QObject::tr("push opcode color"),
        "push"
    }
},
{
    "pop", {
        QObject::tr("pop opcode color"),
        "pop"
    }
},
{
    "crypto", {
        QObject::tr("Cryptographic color"),
        "crypto"
    }
},
{
    "jmp", {
        QObject::tr("jmp instructions color"),
        "jmp"
    }
},
{
    "cjmp", {
        QObject::tr("Color of conditional jump opcodes such as je, jg, jne etc"),
        QObject::tr("Conditional jump")
    }
},
{
    "call", {
        QObject::tr("call instructions color (ccall, rcall, call etc)"),
        "call"
    }
},
{
    "nop", {
        QObject::tr("nop opcode color"),
        "nop"
    }
},
{
    "ret", {
        QObject::tr("ret opcode color"),
        "ret"
    }
},
{
    "trap", {
        QObject::tr("Color of interrupts"),
        QObject::tr("Interrupts")
    }
},
{
    "swi", {
        QObject::tr("swi opcode color"),
        "swi"
    }
},
{
    "cmp", {
        QObject::tr("Color of compare instructions such as test and cmp"),
        QObject::tr("Compare instructions")
    }
},
{
    "reg", {
        QObject::tr("Registers color"),
        QObject::tr("Register")
    }
},
{
    "creg", {
        "",
        "creg"
    }
},
{
    "num", {
        QObject::tr("Color of numeric constants and object pointers"),
        QObject::tr("Constants")
    }
},
{
    "mov", {
        QObject::tr("Color of move instructions such as mov, movd, lea etc"),
        QObject::tr("Move instructions")
    }
},
{
    "func_var", {
        QObject::tr("Function variable color"),
        QObject::tr("Function variable")
    }
},
{
    "func_var_type", {
        QObject::tr("Function variable (local or argument) type color"),
        QObject::tr("Variable type")
    }
},
{
    "func_var_addr", {
        QObject::tr("Function variable address color"),
        QObject::tr("Variable address")
    }
},
{
    "widget_bg", {
        "",
        "widget_bg"
    }
},
{
    "widget_sel", {
        "",
        "widget_sel"
    }
},
{
    "ai.read", {
        "",
        "ai.read"
    }
},
{
    "ai.write", {
        "",
        "ai.write"
    }
},
{
    "ai.exec", {
        "",
        "ai.exec"
    }
},
{
    "ai.seq", {
        "",
        "ai.seq"
    }
},
{
    "ai.ascii", {
        "",
        "ai.ascii"
    }
},
{
    "graph.box", {
        "",
        "graph.box"
    }
},
{
    "graph.box2", {
        "",
        "graph.box2"
    }
},
{
    "graph.box3", {
        "",
        "graph.box3"
    }
},
{
    "graph.box4", {
        "",
        "graph.box4"
    }
},
{
    "graph.true", {
        QObject::tr("In graph view jump arrow true"),
        QObject::tr("Arrow true")
    }
},
{
    "graph.false", {
        QObject::tr("In graph view jump arrow false"),
        QObject::tr("Arrow false")
    }
},
{
    "graph.trufae", {
        QObject::tr("In graph view jump arrow (no condition)"),
        QObject::tr("Arrow")
    }
},
{
    "graph.current", {
        "",
        "graph.current"
    }
},
{
    "graph.traced", {
        "",
        "graph.traced"
    }
},
{
    "gui.overview.node", {
        QObject::tr("Background color of Graph Overview's node"),
        QObject::tr("Graph Overview node")
    }
},
{
    "gui.cflow", {
        "",
        "gui.cflow"
    }
},
{
    "gui.dataoffset", {
        "",
        "gui.dataoffset"
    }
},
{
    "gui.background", {
        QObject::tr("General background color"),
        QObject::tr("Background")
    }
},
{
    "gui.alt_background", {
        QObject::tr("Background color of non-focused graph node"),
        QObject::tr("Node background")
    }
},
{
    "gui.disass_selected", {
        QObject::tr("Background of current graph node"),
        QObject::tr("Current graph node")
    }
},
{
    "gui.border", {
      QObject::tr("Color of node border in graph view"),
      QObject::tr("Node border")
    }
},
{
    "linehl", {
        QObject::tr("Selected line background color"),
        QObject::tr("Line highlight")
    }
},
{
    "wordhl", {
        QObject::tr("Background color of selected word"),
        QObject::tr("Word higlight")
    }
},
{
    "gui.main", {
        QObject::tr("Main function color"),
        QObject::tr("Main")
    }
},
{
    "gui.imports", {
        "",
        "gui.imports"
    }
},
{
    "highlightPC", {
        "",
        "highlightPC"
    }
},
{
    "gui.navbar.err", {
        "",
        "gui.navbar.err"
    }
},
{
    "gui.navbar.seek", {
        "",
        "gui.navbar.seek"
    }
},
{
    "angui.navbar.str", {
        "",
        "angui.navbar.str"
    }
},
{
    "gui.navbar.pc", {
        "",
        "gui.navbar.pc"
    }
},
{
    "gui.navbar.sym", {
        "",
        "gui.navbar.sym"
    }
},
{
    "gui.navbar.code", {
        QObject::tr("Code section color in navigation bar"),
        QObject::tr("Navbar code")
    }
},
{
    "gui.navbar.empty", {
        QObject::tr("Empty section color in navigation bar"),
        QObject::tr("Navbar empty")
    }
},
{
    "ucall", {
        "",
        QObject::tr("ucall")
    }
},
{
    "ujmp", {
        "",
        QObject::tr("ujmp")
    }
},
{
    "gui.breakpoint_background", {
        "",
        QObject::tr("Breakpoint background")
    }
}
};
