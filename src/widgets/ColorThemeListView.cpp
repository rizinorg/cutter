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

    blinkTimer.setInterval(600);
    blinkTimer.start();

    setMouseTracking(true);
}

void ColorThemeListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    ColorOption prev = previous.data(Qt::UserRole).value<ColorOption>();
    Config()->setColor(prev.optionName, prev.color);
    bool isRizinOption = ThemeWorker().getRizinSpecificOptions().contains(prev.optionName);
    if (isRizinOption) {
        Core()->setColor(prev.optionName, prev.color.name());
    }

    QListView::currentChanged(current, previous);
    emit itemChanged(current.data(Qt::UserRole).value<ColorOption>().color);

    // restart the timer
    if (isRizinOption) {
        blinkTimeout();
        blinkTimer.start();
    }
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
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for code comments"),
        QT_TRANSLATE_NOOP("ColorTheme", "Comment") } },
    { "usrcmt",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for user comments"),
        QT_TRANSLATE_NOOP("ColorTheme", "User Comment") } },
    { "args",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for function arguments"),
        QT_TRANSLATE_NOOP("ColorTheme", "Arguments") } },
    { "fname",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for function names"),
        QT_TRANSLATE_NOOP("ColorTheme", "Function Name") } },
    { "floc",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for function locations"),
        QT_TRANSLATE_NOOP("ColorTheme", "Function Location") } },
    { "fline",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for function lines"),
        QT_TRANSLATE_NOOP("ColorTheme", "Function Line") } },
    { "flag",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for flags"),
        QT_TRANSLATE_NOOP("ColorTheme", "Flag") } },
    { "label",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for labels"),
        QT_TRANSLATE_NOOP("ColorTheme", "Label") } },
    { "help",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for help messages"),
        QT_TRANSLATE_NOOP("ColorTheme", "Help") } },
    { "flow",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for control flow"),
        QT_TRANSLATE_NOOP("ColorTheme", "Flow") } },
    { "flow2",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for control flow (alternative)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Flow 2") } },
    { "prompt",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for prompt"),
        QT_TRANSLATE_NOOP("ColorTheme", "Prompt") } },
    { "offset",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for offsets"),
        QT_TRANSLATE_NOOP("ColorTheme", "Offset") } },
    { "input",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for user input"),
        QT_TRANSLATE_NOOP("ColorTheme", "Input") } },
    { "invalid",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for invalid instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Invalid") } },
    { "other",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for other elements"),
        QT_TRANSLATE_NOOP("ColorTheme", "Other") } },
    { "b0x00",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for null bytes (0x00)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Byte 0x00") } },
    { "b0x7f",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for 0x7f bytes"),
        QT_TRANSLATE_NOOP("ColorTheme", "Byte 0x7f") } },
    { "b0xff",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for 0xff bytes"),
        QT_TRANSLATE_NOOP("ColorTheme", "Byte 0xff") } },
    { "math",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for math operations"),
        QT_TRANSLATE_NOOP("ColorTheme", "Math") } },
    { "bin",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for binary information"),
        QT_TRANSLATE_NOOP("ColorTheme", "Binary") } },
    { "btext",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for text in binary"),
        QT_TRANSLATE_NOOP("ColorTheme", "Text") } },
    { "push",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for push instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Push") } },
    { "pop",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for pop instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Pop") } },
    { "crypto",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for crypto instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Crypto") } },
    { "jmp",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for jump instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Jump") } },
    { "cjmp",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for conditional jumps"),
        QT_TRANSLATE_NOOP("ColorTheme", "Conditional Jump") } },
    { "call",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for call instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Call") } },
    { "nop",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for nop instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Nop") } },
    { "ret",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for return instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Return") } },
    { "trap",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for trap/interrupt instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Trap") } },
    { "ucall",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for unknown calls"),
        QT_TRANSLATE_NOOP("ColorTheme", "Unknown Call") } },
    { "ujmp",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for unknown jumps"),
        QT_TRANSLATE_NOOP("ColorTheme", "Unknown Jump") } },
    { "swi",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for software interrupts"),
        QT_TRANSLATE_NOOP("ColorTheme", "Software Interrupt") } },
    { "cmp",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for compare instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Compare") } },
    { "reg",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for registers"),
        QT_TRANSLATE_NOOP("ColorTheme", "Register") } },
    { "creg",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for changed registers"),
        QT_TRANSLATE_NOOP("ColorTheme", "Changed Register") } },
    { "num",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for numbers"),
        QT_TRANSLATE_NOOP("ColorTheme", "Number") } },
    { "mov",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for move instructions"),
        QT_TRANSLATE_NOOP("ColorTheme", "Move") } },
    { "func_var",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for function variables"),
        QT_TRANSLATE_NOOP("ColorTheme", "Function Variable") } },
    { "func_var_type",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for function variable types"),
        QT_TRANSLATE_NOOP("ColorTheme", "Variable Type") } },
    { "func_var_addr",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for function variable addresses"),
        QT_TRANSLATE_NOOP("ColorTheme", "Variable Address") } },
    { "widget_bg",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for widget background"),
        QT_TRANSLATE_NOOP("ColorTheme", "Widget Background") } },
    { "widget_sel",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for selected widget"),
        QT_TRANSLATE_NOOP("ColorTheme", "Widget Selection") } },
    { "meta",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for metadata"),
        QT_TRANSLATE_NOOP("ColorTheme", "Metadata") } },
    { "ai.read",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for memory read access"),
        QT_TRANSLATE_NOOP("ColorTheme", "AI Read") } },
    { "ai.write",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for memory write access"),
        QT_TRANSLATE_NOOP("ColorTheme", "AI Write") } },
    { "ai.exec",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for executable memory"),
        QT_TRANSLATE_NOOP("ColorTheme", "AI Exec") } },
    { "ai.seq",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for sequential memory"),
        QT_TRANSLATE_NOOP("ColorTheme", "AI Sequence") } },
    { "ai.ascii",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for ASCII in memory"),
        QT_TRANSLATE_NOOP("ColorTheme", "AI ASCII") } },
    { "graph.box",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for graph box"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Box") } },
    { "graph.box2",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for graph box (alternative 2)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Box 2") } },
    { "graph.box3",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for graph box (alternative 3)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Box 3") } },
    { "graph.box4",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for graph box (alternative 4)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Box 4") } },
    { "graph.true",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for true branch in graph"),
        QT_TRANSLATE_NOOP("ColorTheme", "Arrow True") } },
    { "graph.false",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for false branch in graph"),
        QT_TRANSLATE_NOOP("ColorTheme", "Arrow False") } },
    { "graph.trufae",
      { QT_TRANSLATE_NOOP("ColorTheme", "In graph view jump arrow (no condition)"),
        QT_TRANSLATE_NOOP("ColorTheme", "Arrow") } },
    { "graph.ujump",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for unknown jump in graph"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Undefined Jump") } },
    { "graph.current",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for current node in graph"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Current") } },
    { "graph.traced",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for traced node in graph"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Traced") } },
    { "diff.unknown",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for unknown diff"),
        QT_TRANSLATE_NOOP("ColorTheme", "Diff Unknown") } },
    { "diff.new",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for new diff"),
        QT_TRANSLATE_NOOP("ColorTheme", "Diff New") } },
    { "diff.match",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for matched diff"),
        QT_TRANSLATE_NOOP("ColorTheme", "Diff Match") } },
    { "diff.unmatch",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for unmatched diff"),
        QT_TRANSLATE_NOOP("ColorTheme", "Diff Unmatch") } },
    { "gui.overview.node",
      { QT_TRANSLATE_NOOP("ColorTheme", "Background color of Graph Overview's node"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Overview Node") } },
    { "gui.overview.fill",
      { QT_TRANSLATE_NOOP("ColorTheme", "Fill color of Graph Overview's selection"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Overview Fill") } },
    { "gui.overview.border",
      { QT_TRANSLATE_NOOP("ColorTheme", "Border color of Graph Overview's selection"),
        QT_TRANSLATE_NOOP("ColorTheme", "Graph Overview Border") } },
    { "gui.cflow",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for GUI control flow"),
        QT_TRANSLATE_NOOP("ColorTheme", "Control Flow") } },
    { "gui.dataoffset",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for GUI data offset"),
        QT_TRANSLATE_NOOP("ColorTheme", "Data Offset") } },
    { "gui.background",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for GUI background"),
        QT_TRANSLATE_NOOP("ColorTheme", "Background") } },
    { "gui.alt_background",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for GUI alternate background"),
        QT_TRANSLATE_NOOP("ColorTheme", "Node Background") } },
    { "gui.disass_selected",
      { QT_TRANSLATE_NOOP("ColorTheme", "Background of current graph node"),
        QT_TRANSLATE_NOOP("ColorTheme", "Current Node") } },
    { "gui.border",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for GUI border"),
        QT_TRANSLATE_NOOP("ColorTheme", "Node Border") } },
    { "lineHighlight",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for highlighted line"),
        QT_TRANSLATE_NOOP("ColorTheme", "Line Highlight") } },
    { "wordHighlightBg",
      { QT_TRANSLATE_NOOP("ColorTheme", "Background color for highlighted word"),
        QT_TRANSLATE_NOOP("ColorTheme", "Word Highlight Background") } },
    { "wordHighlightFg",
      { QT_TRANSLATE_NOOP("ColorTheme", "Foreground color for highlighted word"),
        QT_TRANSLATE_NOOP("ColorTheme", "Word Highlight Foreground") } },
    { "gui.main",
      { QT_TRANSLATE_NOOP("ColorTheme", "Main function color"),
        QT_TRANSLATE_NOOP("ColorTheme", "Main Function") } },
    { "gui.imports",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for imported symbols"),
        QT_TRANSLATE_NOOP("ColorTheme", "Imports") } },
    { "highlightPC",
      { QT_TRANSLATE_NOOP("ColorTheme", "Color for Program Counter highlight"),
        QT_TRANSLATE_NOOP("ColorTheme", "PC Highlight") } },
    { "gui.navbar.err",
      { QT_TRANSLATE_NOOP("ColorTheme", "Error color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar Error") } },
    { "gui.navbar.seek",
      { QT_TRANSLATE_NOOP("ColorTheme", "Seek color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar Seek") } },
    { "gui.navbar.str",
      { QT_TRANSLATE_NOOP("ColorTheme", "String color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar String") } },
    { "gui.navbar.pc",
      { QT_TRANSLATE_NOOP("ColorTheme", "PC position color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar PC") } },
    { "gui.navbar.sym",
      { QT_TRANSLATE_NOOP("ColorTheme", "Symbol color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar Symbol") } },
    { "gui.navbar.code",
      { QT_TRANSLATE_NOOP("ColorTheme", "Code section color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar Code") } },
    { "gui.navbar.import",
      { QT_TRANSLATE_NOOP("ColorTheme", "Import section color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar Import") } },
    { "gui.navbar.signature",
      { QT_TRANSLATE_NOOP("ColorTheme", "Signature section color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar Signature") } },
    { "gui.navbar.data",
      { QT_TRANSLATE_NOOP("ColorTheme", "Data section color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar Data") } },
    { "gui.navbar.unexplored",
      { QT_TRANSLATE_NOOP("ColorTheme", "Unexplored section color in navigation bar"),
        QT_TRANSLATE_NOOP("ColorTheme", "Navbar Unexplored") } },
    { "gui.breakpoint_background",
      { QT_TRANSLATE_NOOP("ColorTheme", "Background color for breakpoints"),
        QT_TRANSLATE_NOOP("ColorTheme", "Breakpoint Background") } },
};
}
