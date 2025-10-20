#include <QDebug>
#include <QJsonObject>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QScreen>
#include <QJsonArray>
#include <QScrollBar>
#include <QApplication>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QSortFilterProxyModel>

#include "common/Configuration.h"
#include "common/ColorThemeWorker.h"
#include "common/Helpers.h"

#include "widgets/ColorThemeListView.h"

constexpr int allFieldsRole = Qt::UserRole + 2;

struct OptionInfo
{
    const char *info;
    const char *displayingtext;
};

namespace {
extern const QMap<QString, OptionInfo> OPTION_INFO_MAP;
}

ColorOptionDelegate::ColorOptionDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    resetButtonPixmap = getPixmapFromSvg(":/img/icons/reset.svg", qApp->palette().text().color());
    connect(qApp, &QGuiApplication::paletteChanged, this, [this]() {
        resetButtonPixmap =
                getPixmapFromSvg(":/img/icons/reset.svg", qApp->palette().text().color());
    });
}

void ColorOptionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    int margin = this->margin;
    painter->save();
    painter->setFont(option.font);
    painter->setRenderHint(QPainter::Antialiasing);

    ColorOption currCO = index.data(Qt::UserRole).value<ColorOption>();

    QFontMetrics fm = QFontMetrics(painter->font());
    int penWidth = painter->pen().width();
    int fontHeight = fm.height();
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
    descTextRect.setTopLeft(colorRect.topRight()
                            + QPoint(margin, colorRect.height() / 2 - fontHeight / 2));
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
        auto self = const_cast<ColorOptionDelegate *>(this);
        self->resetButtonRect = resetButtonRect;
    }
    if (option.rect.contains(this->resetButtonRect) && this->resetButtonRect != resetButtonRect) {
        auto self = const_cast<ColorOptionDelegate *>(this);
        self->resetButtonRect = QRect(0, 0, 0, 0);
    }

    painter->setPen(qApp->palette().text().color());

    QFontMetrics fm2 = QFontMetrics(painter->font());
    auto info = OPTION_INFO_MAP[currCO.optionName];
    QString name = fm2.elidedText(QApplication::translate("ColorTheme", info.displayingtext),
                                  Qt::ElideRight, optionNameRect.width());
    painter->drawText(optionNameRect, name);

    QPainterPath roundedOptionRect;
    roundedOptionRect.addRoundedRect(optionRect, fontHeight / 4, fontHeight / 4);
    painter->setPen(qApp->palette().text().color());
    painter->drawPath(roundedOptionRect);

    QPainterPath roundedColorRect;
    roundedColorRect.addRoundedRect(colorRect, fontHeight / 4, fontHeight / 4);
    // Create chess-like pattern of black and white squares
    // and fill background of roundedColorRect with it
    if (currCO.color.alpha() < 255) {
        const int c1 = static_cast<int>(8);
        const int c2 = c1 / 2;
        QPixmap p(c1, c1);
        QPainter paint(&p);
        paint.fillRect(0, 0, c1, c1, Qt::white);
        paint.fillRect(0, 0, c2, c2, Qt::black);
        paint.fillRect(c2, c2, c2, c2, Qt::black);
        QBrush b;
        b.setTexture(p);
        painter->fillPath(roundedColorRect, b);
    }
    painter->setPen(currCO.color);
    painter->fillPath(roundedColorRect, currCO.color);

    QFontMetrics fm3 = QFontMetrics(painter->font());
    QString desc = fm3.elidedText(currCO.optionName + ": "
                                          + QApplication::translate("ColorTheme", info.info),
                                  Qt::ElideRight, descTextRect.width());
    painter->setPen(qApp->palette().text().color());
    painter->setBrush(qApp->palette().text());
    painter->drawText(descTextRect, desc);

    painter->restore();
}

QSize ColorOptionDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    qreal margin = this->margin;
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

QPixmap ColorOptionDelegate::getPixmapFromSvg(const QString &fileName, const QColor &after) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QPixmap();
    }
    QString data = file.readAll();
    data.replace(QRegularExpression("#[0-9a-fA-F]{6}"), QString("%1").arg(after.name()));

    QSvgRenderer svgRenderer(data.toUtf8());
    QFontMetrics fm = QFontMetrics(qApp->font());
    QPixmap pix(QSize(fm.height(), fm.height()));
    pix.fill(Qt::transparent);

    QPainter pixPainter(&pix);
    svgRenderer.render(&pixPainter);

    return pix;
}

ColorThemeListView::ColorThemeListView(QWidget *parent) : QListView(parent)
{
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    ColorSettingsModel *model = new ColorSettingsModel(this);
    proxy->setSourceModel(model);
    model->updateTheme();
    setModel(proxy);
    proxy->setFilterRole(allFieldsRole);
    proxy->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    proxy->setSortRole(Qt::DisplayRole);
    proxy->setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    setItemDelegate(new ColorOptionDelegate(this));
    setResizeMode(ResizeMode::Adjust);

    backgroundColor = colorSettingsModel()->getTheme().find("gui.background").value();

    connect(&blinkTimer, &QTimer::timeout, this, &ColorThemeListView::blinkTimeout);

    blinkTimer.setInterval(400);
    blinkTimer.start();

    setMouseTracking(true);
}

void ColorThemeListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    ColorOption prev = previous.data(Qt::UserRole).value<ColorOption>();
    Config()->setColor(prev.optionName, prev.color);
    if (ThemeWorker().getRizinSpecificOptions().contains(prev.optionName)) {
        Core()->setColor(prev.optionName, prev.color.name());
    }

    QListView::currentChanged(current, previous);
    emit itemChanged(current.data(Qt::UserRole).value<ColorOption>().color);
}

void ColorThemeListView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                     const QVector<int> &roles)
{
    ColorOption curr = topLeft.data(Qt::UserRole).value<ColorOption>();
    if (curr.optionName == "gui.background") {
        backgroundColor = curr.color;
    }
    QListView::dataChanged(topLeft, bottomRight, roles);
    emit itemChanged(curr.color);
    emit dataChanged(curr);
}

void ColorThemeListView::mouseReleaseEvent(QMouseEvent *e)
{
    if (qobject_cast<ColorOptionDelegate *>(itemDelegate())
                ->getResetButtonRect()
                .contains(e->pos())) {
        ColorOption co = currentIndex().data(Qt::UserRole).value<ColorOption>();
        co.changed = false;
        co.color = ThemeWorker().getTheme(Config()->getColorTheme())[co.optionName];
        model()->setData(currentIndex(), QVariant::fromValue(co));
        QCursor c;
        c.setShape(Qt::CursorShape::ArrowCursor);
        setCursor(c);
    }
}

void ColorThemeListView::mouseMoveEvent(QMouseEvent *e)
{
    if (qobject_cast<ColorOptionDelegate *>(itemDelegate())
                ->getResetButtonRect()
                .contains(e->pos())) {
        QCursor c;
        c.setShape(Qt::CursorShape::PointingHandCursor);
        setCursor(c);
    } else if (cursor().shape() == Qt::CursorShape::PointingHandCursor) {
        QCursor c;
        c.setShape(Qt::CursorShape::ArrowCursor);
        setCursor(c);
    }
}

ColorSettingsModel *ColorThemeListView::colorSettingsModel() const
{
    return static_cast<ColorSettingsModel *>(
            static_cast<QSortFilterProxyModel *>(model())->sourceModel());
}

void ColorThemeListView::blinkTimeout()
{
    static enum { Normal, Invisible } state = Normal;
    state = state == Normal ? Invisible : Normal;
    backgroundColor.setAlphaF(1);

    auto updateColor = [](const QString &name, const QColor &color) {
        Config()->setColor(name, color);
        if (ThemeWorker().getRizinSpecificOptions().contains(name)) {
            Core()->setColor(name, color.name());
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

ColorSettingsModel::ColorSettingsModel(QObject *parent) : QAbstractListModel(parent) {}

QVariant ColorSettingsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() < 0 || index.row() >= theme.size()) {
        return QVariant();
    }

    const QString key = theme.at(index.row()).optionName;
    auto info = OPTION_INFO_MAP[key];

    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(QApplication::translate("ColorTheme", info.displayingtext));
    }

    if (role == Qt::UserRole) {
        return QVariant::fromValue(theme.at(index.row()));
    }

    if (role == Qt::ToolTipRole) {
        return QVariant::fromValue(QApplication::translate("ColorTheme", info.info));
    }

    if (role == allFieldsRole) {
        const QString name = key;
        return QVariant::fromValue(QApplication::translate("ColorTheme", info.displayingtext) + " "
                                   + QApplication::translate("ColorTheme", info.info) + " " + name);
    }

    return QVariant();
}

bool ColorSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
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
    beginResetModel();
    theme.clear();
    ColorThemeWorker::Theme obj = ThemeWorker().getTheme(Config()->getColorTheme());

    for (auto it = obj.constBegin(); it != obj.constEnd(); it++) {
        theme.push_back({ it.key(), it.value(), false });
    }

    std::sort(theme.begin(), theme.end(), [](const ColorOption &f, const ColorOption &s) {
        QString s1 = f.optionName;
        QString s2 = s.optionName;
        int r = s1.compare(s2, Qt::CaseSensitivity::CaseInsensitive);
        return r < 0;
    });
    endResetModel();
}

ColorThemeWorker::Theme ColorSettingsModel::getTheme() const
{
    ColorThemeWorker::Theme th;
    for (auto &it : theme) {
        th.insert(it.optionName, it.color);
    }
    return th;
}

namespace {
const QMap<QString, OptionInfo> OPTION_INFO_MAP = {
    { "comment",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of comment generated by Rizin"),
        QT_TRANSLATE_NOOP("ColorTheme", "Comment") } },
    { "usrcmt",
      { QT_TRANSLATE_NOOP("ColorTheme", "Comment created by user"),
        QT_TRANSLATE_NOOP("ColorTheme", "Color of user Comment") } },
    { "args", { "", QT_TRANSLATE_NOOP("ColorTheme", "args") } },
    { "fname",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of names of functions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Function name") } },
    { "floc",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of function location"),
        QT_TRANSLATE_NOOP("ColorTheme", "Function location") } },
    { "fline",
      { QT_TRANSLATE_NOOP("ColorTheme",
                          "Color of the line which shows which opcodes belongs to a function"),
        QT_TRANSLATE_NOOP("ColorTheme", "Function line") } },
    { "flag",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of flags (similar to bookmarks for offset)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Flag") } },
    { "label", { "", QT_TRANSLATE_NOOP("ColorTheme", "Label") } },
    { "help", { "", QT_TRANSLATE_NOOP("ColorTheme", "Help") } },
    { "flow",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of lines showing jump destination"),
        QT_TRANSLATE_NOOP("ColorTheme", "Flow") } },
    { "flow2", { "", QT_TRANSLATE_NOOP("ColorTheme", "flow2") } },
    { "prompt",
      { QT_TRANSLATE_NOOP("ColorTheme", "Info"), QT_TRANSLATE_NOOP("ColorTheme", "prompt") } },
    { "offset",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of offsets"),
        QT_TRANSLATE_NOOP("ColorTheme", "Offset") } },
    { "input",
      { QT_TRANSLATE_NOOP("ColorTheme", "Info"), QT_TRANSLATE_NOOP("ColorTheme", "input") } },
    { "invalid",
      { QT_TRANSLATE_NOOP("ColorTheme", "Invalid opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "invalid") } },
    { "other", { "", QT_TRANSLATE_NOOP("ColorTheme", "other") } },
    { "b0x00",
      { QT_TRANSLATE_NOOP("ColorTheme", "0x00 opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "b0x00") } },
    { "b0x7f",
      { QT_TRANSLATE_NOOP("ColorTheme", "0x7f opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "b0x7f") } },
    { "b0xff",
      { QT_TRANSLATE_NOOP("ColorTheme", "0xff opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "b0xff") } },
    { "math",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of arithmetic opcodes (add, div, mul etc)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Arithmetic") } },
    { "bin",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of binary operations (and, or, xor etc)."),
        QT_TRANSLATE_NOOP("ColorTheme", "Binary") } },
    { "btext",
      { QT_TRANSLATE_NOOP(
                "ColorTheme",
                "Color of object names, commas between operators, squared brackets and operators "
                "inside them."),
        QT_TRANSLATE_NOOP("ColorTheme", "Text") } },
    { "push",
      { QT_TRANSLATE_NOOP("ColorTheme", "push opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "push") } },
    { "pop",
      { QT_TRANSLATE_NOOP("ColorTheme", "pop opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "pop") } },
    { "crypto",
      { QT_TRANSLATE_NOOP("ColorTheme", "Cryptographic color"),
        QT_TRANSLATE_NOOP("ColorTheme", "crypto") } },
    { "jmp",
      { QT_TRANSLATE_NOOP("ColorTheme", "jmp instructions color"),
        QT_TRANSLATE_NOOP("ColorTheme", "jmp") } },
    { "cjmp",
      { QT_TRANSLATE_NOOP("ColorTheme",
                          "Color of conditional jump opcodes such as je, jg, jne etc"),
        QT_TRANSLATE_NOOP("ColorTheme", "Conditional jump") } },
    { "call",
      { QT_TRANSLATE_NOOP("ColorTheme", "call instructions color (ccall, rcall, call etc)"),
        QT_TRANSLATE_NOOP("ColorTheme", "call") } },
    { "nop",
      { QT_TRANSLATE_NOOP("ColorTheme", "nop opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "nop") } },
    { "ret",
      { QT_TRANSLATE_NOOP("ColorTheme", "ret opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "ret") } },
    { "trap",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of interrupts"),
        QT_TRANSLATE_NOOP("ColorTheme", "Interrupts") } },
    { "swi",
      { QT_TRANSLATE_NOOP("ColorTheme", "swi opcode color"),
        QT_TRANSLATE_NOOP("ColorTheme", "swi") } },
    { "cmp",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of compare instructions such as test and cmp"),
        QT_TRANSLATE_NOOP("ColorTheme", "Compare instructions") } },
    { "reg",
      { QT_TRANSLATE_NOOP("ColorTheme", "Registers color"),
        QT_TRANSLATE_NOOP("ColorTheme", "Register") } },
    { "creg", { "", QT_TRANSLATE_NOOP("ColorTheme", "creg") } },
    { "num",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of numeric constants and object pointers"),
        QT_TRANSLATE_NOOP("ColorTheme", "Constants") } },
    { "mov",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of move instructions such as mov, movd, lea etc"),
        QT_TRANSLATE_NOOP("ColorTheme", "Move instructions") } },
    { "func_var",
      { QT_TRANSLATE_NOOP("ColorTheme", "Function variable color"),
        QT_TRANSLATE_NOOP("ColorTheme", "Function variable") } },
    { "func_var_type",
      { QT_TRANSLATE_NOOP("ColorTheme", "Function variable (local or argument) type color"),
        QT_TRANSLATE_NOOP("ColorTheme", "Variable type") } },
    { "func_var_addr",
      { QT_TRANSLATE_NOOP("ColorTheme", "Function variable address color"),
        QT_TRANSLATE_NOOP("ColorTheme", "Variable address") } },
    { "widget_bg", { "", QT_TRANSLATE_NOOP("ColorTheme", "widget_bg") } },
    { "widget_sel", { "", QT_TRANSLATE_NOOP("ColorTheme", "widget_sel") } },
    { "ai.read", { "", QT_TRANSLATE_NOOP("ColorTheme", "ai.read") } },
    { "ai.write", { "", QT_TRANSLATE_NOOP("ColorTheme", "ai.write") } },
    { "ai.exec", { "", QT_TRANSLATE_NOOP("ColorTheme", "ai.exec") } },
    { "ai.seq", { "", QT_TRANSLATE_NOOP("ColorTheme", "ai.seq") } },
    { "ai.ascii", { "", QT_TRANSLATE_NOOP("ColorTheme", "ai.ascii") } },
    { "graph.box", { "", QT_TRANSLATE_NOOP("ColorTheme", "graph.box") } },
    { "graph.box2", { "", QT_TRANSLATE_NOOP("ColorTheme", "graph.box2") } },
    { "graph.box3", { "", QT_TRANSLATE_NOOP("ColorTheme", "graph.box3") } },
    { "graph.box4", { "", QT_TRANSLATE_NOOP("ColorTheme", "graph.box4") } },
    { "graph.true",
      { QT_TRANSLATE_NOOP("ColorTheme", "In graph view jump arrow true"),
        QT_TRANSLATE_NOOP("ColorTheme", "Arrow true") } },
    { "graph.false",
      { QT_TRANSLATE_NOOP("ColorTheme", "In graph view jump arrow false"),
        QT_TRANSLATE_NOOP("ColorTheme", "Arrow false") } },
    { "graph.trufae",
      { QT_TRANSLATE_NOOP("ColorTheme", "In graph view jump arrow (no condition)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Arrow") } },
    { "graph.current", { "", QT_TRANSLATE_NOOP("ColorTheme", "graph.current") } },
    { "graph.traced", { "", QT_TRANSLATE_NOOP("ColorTheme", "graph.traced") } },
    { "gui.overview.node",
      { QT_TRANSLATE_NOOP("ColorTheme", "Background color of Graph Overview's node"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Overview node") } },
    { "gui.overview.fill",
      { QT_TRANSLATE_NOOP("ColorTheme", "Fill color of Graph Overview's selection"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Overview fill") } },
    { "gui.overview.border",
      { QT_TRANSLATE_NOOP("ColorTheme", "Border color of Graph Overview's selection"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Overview border") } },
    { "gui.cflow", { "", QT_TRANSLATE_NOOP("ColorTheme", "gui.cflow") } },
    { "gui.dataoffset", { "", QT_TRANSLATE_NOOP("ColorTheme", "gui.dataoffset") } },
    { "gui.background",
      { QT_TRANSLATE_NOOP("ColorTheme", "General background color"),
        QT_TRANSLATE_NOOP("ColorTheme", "Background") } },
    { "gui.alt_background",
      { QT_TRANSLATE_NOOP("ColorTheme", "Background color of non-focused graph node"),
        QT_TRANSLATE_NOOP("ColorTheme", "Node background") } },
    { "gui.disass_selected",
      { QT_TRANSLATE_NOOP("ColorTheme", "Background of current graph node"),
        QT_TRANSLATE_NOOP("ColorTheme", "Current graph node") } },
    { "gui.border",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color of node border in graph view"),
        QT_TRANSLATE_NOOP("ColorTheme", "Node border") } },
    { "lineHighlight",
      { QT_TRANSLATE_NOOP("ColorTheme", "Selected line background color"),
        QT_TRANSLATE_NOOP("ColorTheme", "Line highlight") } },
    { "wordHighlight",
      { QT_TRANSLATE_NOOP("ColorTheme", "Background color of selected word"),
        QT_TRANSLATE_NOOP("ColorTheme", "Word highlight") } },
    { "gui.main",
      { QT_TRANSLATE_NOOP("ColorTheme", "Main function color"),
        QT_TRANSLATE_NOOP("ColorTheme", "Main") } },
    { "gui.imports", { "", QT_TRANSLATE_NOOP("ColorTheme", "gui.imports") } },
    { "highlightPC", { "", QT_TRANSLATE_NOOP("ColorTheme", "highlightPC") } },
    { "gui.navbar.err", { "", QT_TRANSLATE_NOOP("ColorTheme", "gui.navbar.err") } },
    { "gui.navbar.seek", { "", QT_TRANSLATE_NOOP("ColorTheme", "gui.navbar.seek") } },
    { "angui.navbar.str", { "", QT_TRANSLATE_NOOP("ColorTheme", "angui.navbar.str") } },
    { "gui.navbar.pc", { "", QT_TRANSLATE_NOOP("ColorTheme", "gui.navbar.pc") } },
    { "gui.navbar.sym", { "", QT_TRANSLATE_NOOP("ColorTheme", "gui.navbar.sym") } },
    { "gui.navbar.code",
      { QT_TRANSLATE_NOOP("ColorTheme", "Code section color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar code") } },
    { "gui.navbar.empty",
      { QT_TRANSLATE_NOOP("ColorTheme", "Empty section color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar empty") } },
    { "ucall", { "", QT_TRANSLATE_NOOP("ColorTheme", "ucall") } },
    { "ujmp", { "", QT_TRANSLATE_NOOP("ColorTheme", "ujmp") } },
    { "gui.breakpoint_background",
      { "", QT_TRANSLATE_NOOP("ColorTheme", "Breakpoint background") } }
};
}