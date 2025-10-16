#include "CutterGraphView.h"

#include "core/Cutter.h"
#include "common/Configuration.h"
#include "dialogs/MultitypeFileSaveDialog.h"
#include "TempConfig.h"
#include "shortcuts/ShortcutManager.h"

#include <cmath>

#include <QStandardPaths>
#include <QActionGroup>

static const uint64_t BITMPA_EXPORT_WARNING_SIZE = 32 * 1024 * 1024;

#ifndef NDEBUG
#    define GRAPH_GRID_DEBUG_MODES true
#else
#    define GRAPH_GRID_DEBUG_MODES false
#endif

CutterGraphView::CutterGraphView(QWidget *parent)
    : GraphView(parent),
      mFontMetrics(nullptr),
      actionExportGraph(tr("Export Graph"), this),
      graphLayout(GraphView::Layout::GridMedium)
{
    connect(Core(), &CutterCore::graphOptionsChanged, this, &CutterGraphView::refreshView);
    connect(Config(), &Configuration::colorsUpdated, this, &CutterGraphView::colorsUpdatedSlot);
    connect(Config(), &Configuration::fontsUpdated, this, &CutterGraphView::fontsUpdatedSlot);

    initFont();
    updateColors();

    connect(&actionExportGraph, &QAction::triggered, this, &CutterGraphView::showExportDialog);

    layoutMenu = new QMenu(tr("Layout"), this);
    horizontalLayoutAction = layoutMenu->addAction(tr("Horizontal"));
    horizontalLayoutAction->setCheckable(true);

    static const std::pair<QString, GraphView::Layout> LAYOUT_CONFIG[] = {
        { tr("Grid narrow"), GraphView::Layout::GridNarrow },
        { tr("Grid medium"), GraphView::Layout::GridMedium },
        { tr("Grid wide"), GraphView::Layout::GridWide }
#if GRAPH_GRID_DEBUG_MODES
        ,
        { "GridAAA", GraphView::Layout::GridAAA },
        { "GridAAB", GraphView::Layout::GridAAB },
        { "GridABA", GraphView::Layout::GridABA },
        { "GridABB", GraphView::Layout::GridABB },
        { "GridBAA", GraphView::Layout::GridBAA },
        { "GridBAB", GraphView::Layout::GridBAB },
        { "GridBBA", GraphView::Layout::GridBBA },
        { "GridBBB", GraphView::Layout::GridBBB }
#endif
#ifdef CUTTER_ENABLE_GRAPHVIZ
        ,
        { tr("Graphviz polyline"), GraphView::Layout::GraphvizPolyline },
        { tr("Graphviz ortho"), GraphView::Layout::GraphvizOrtho },
        { tr("Graphviz sfdp"), GraphView::Layout::GraphvizSfdp },
        { tr("Graphviz neato"), GraphView::Layout::GraphvizNeato },
        { tr("Graphviz twopi"), GraphView::Layout::GraphvizTwoPi },
        { tr("Graphviz circo"), GraphView::Layout::GraphvizCirco }
#endif
    };
    layoutMenu->addSeparator();
    connect(horizontalLayoutAction, &QAction::toggled, this, &CutterGraphView::updateLayout);
    QActionGroup *layoutGroup = new QActionGroup(layoutMenu);
    for (auto &item : LAYOUT_CONFIG) {
        auto action = layoutGroup->addAction(item.first);
        action->setCheckable(true);
        GraphView::Layout layout = item.second;
        if (layout == this->graphLayout) {
            action->setChecked(true);
        }
        connect(action, &QAction::triggered, this, [this, layout]() {
            this->graphLayout = layout;
            updateLayout();
        });
    }
    layoutMenu->addActions(layoutGroup->actions());

    grabGesture(Qt::PinchGesture);
}

QPoint CutterGraphView::getTextOffset(int line) const
{
    return QPoint(padding, padding + line * charHeight);
}

void CutterGraphView::initFont()
{
    setFont(Config()->getFont());
    QFontMetricsF metrics(font());
    baseline = int(metrics.ascent());
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    ACharWidth = metrics.width('A');
#else
    ACharWidth = metrics.horizontalAdvance('A');
#endif
    padding = ACharWidth;
    charHeight = static_cast<int>(metrics.height());
    charOffset = 0;
    mFontMetrics.reset(new CachedFontMetrics<qreal>(font()));
}

void CutterGraphView::zoom(QPointF mouseRelativePos, double velocity)
{
    qreal newScale = getViewScale() * std::pow(1.25, velocity);
    setZoom(mouseRelativePos, newScale);
}

void CutterGraphView::setZoom(QPointF mouseRelativePos, double scale)
{
    mouseRelativePos.rx() *= size().width();
    mouseRelativePos.ry() *= size().height();
    mouseRelativePos /= getViewScale();

    auto globalMouse = mouseRelativePos + getViewOffset();
    mouseRelativePos *= getViewScale();
    qreal newScale = scale;
    newScale = std::max(newScale, 0.05);
    mouseRelativePos /= newScale;
    setViewScale(newScale);

    // Adjusting offset, so that zooming will be approaching to the cursor.
    setViewOffset(globalMouse.toPoint() - mouseRelativePos.toPoint());

    viewport()->update();
    emit viewZoomed();
}

void CutterGraphView::zoomIn()
{
    zoom(QPointF(0.5, 0.5), 1);
}

void CutterGraphView::zoomOut()
{
    zoom(QPointF(0.5, 0.5), -1);
}

void CutterGraphView::zoomReset()
{
    setZoom(QPointF(0.5, 0.5), 1);
}

void CutterGraphView::showExportDialog()
{
    showExportGraphDialog("global_funcall", RZ_CORE_GRAPH_TYPE_FUNCALL, RVA_INVALID);
}

void CutterGraphView::updateColors()
{
    disassemblyBackgroundColor = ConfigColor("gui.alt_background");
    disassemblySelectedBackgroundColor = ConfigColor("gui.disass_selected");
    mDisabledBreakpointColor = disassemblyBackgroundColor;
    graphNodeColor = ConfigColor("gui.border");
    backgroundColor = ConfigColor("gui.background");
    disassemblySelectionColor = ConfigColor("lineHighlight");
    PCSelectionColor = ConfigColor("highlightPC");

    jmpColor = ConfigColor("graph.trufae");
    brtrueColor = ConfigColor("graph.true");
    brfalseColor = ConfigColor("graph.false");

    mCommentColor = ConfigColor("comment");
}

void CutterGraphView::colorsUpdatedSlot()
{
    updateColors();
    refreshView();
}

GraphLayout::LayoutConfig CutterGraphView::getLayoutConfig()
{
    auto blockSpacing = Config()->getGraphBlockSpacing();
    auto edgeSpacing = Config()->getGraphEdgeSpacing();
    GraphLayout::LayoutConfig layoutConfig;
    layoutConfig.blockHorizontalSpacing = blockSpacing.x();
    layoutConfig.blockVerticalSpacing = blockSpacing.y();
    layoutConfig.edgeHorizontalSpacing = edgeSpacing.x();
    layoutConfig.edgeVerticalSpacing = edgeSpacing.y();
    return layoutConfig;
}

void CutterGraphView::updateLayout()
{
    setGraphLayout(GraphView::makeGraphLayout(graphLayout, horizontalLayoutAction->isChecked()));
    saveCurrentBlock();
    setLayoutConfig(getLayoutConfig());
    computeGraphPlacement();
    restoreCurrentBlock();
    emit viewRefreshed();
}

void CutterGraphView::fontsUpdatedSlot()
{
    initFont();
    refreshView();
}

bool CutterGraphView::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        qhelpers::KeyComb key = Qt::Key(keyEvent->key()) | keyEvent->modifiers();
        if (Shortcuts()->matchesKeySequence("General.zoomOut", key)
            || Shortcuts()->matchesKeySequence("General.zoomReset", key)
            || Shortcuts()->matchesKeySequence("General.zoomIn", key)) {
            event->accept();
            return true;
        }
        break;
    }
    case QEvent::KeyPress: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        qhelpers::KeyComb key = Qt::Key(keyEvent->key()) | keyEvent->modifiers();
        if (Shortcuts()->matchesKeySequence("General.zoomIn", key)) {
            zoomIn();
            return true;
        } else if (Shortcuts()->matchesKeySequence("General.zoomOut", key)) {
            zoomOut();
            return true;
        } else if (Shortcuts()->matchesKeySequence("General.zoomReset", key)) {
            zoomReset();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return GraphView::event(event);
}

void CutterGraphView::refreshView()
{
    initFont();
    setLayoutConfig(getLayoutConfig());
}

bool CutterGraphView::gestureEvent(QGestureEvent *event)
{
    if (!event) {
        return false;
    }

    if (auto gesture = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture))) {
        auto changeFlags = gesture->changeFlags();

        if (changeFlags & QPinchGesture::ScaleFactorChanged) {
            auto cursorPos = gesture->centerPoint();
            cursorPos.rx() /= size().width();
            cursorPos.ry() /= size().height();

            setZoom(cursorPos, getViewScale() * gesture->scaleFactor());
        }

        event->accept(gesture);
        return true;
    }

    return false;
}

void CutterGraphView::wheelEvent(QWheelEvent *event)
{
    // when CTRL is pressed, we zoom in/out with mouse wheel
    if (Qt::ControlModifier == event->modifiers()) {
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            int numSteps = numDegrees.y() / 15;

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
            QPointF relativeMousePos = event->pos();
#else
            QPointF relativeMousePos = event->position();
#endif
            relativeMousePos.rx() /= size().width();
            relativeMousePos.ry() /= size().height();

            zoom(relativeMousePos, numSteps);
        }
        event->accept();
    } else {
        // use mouse wheel for scrolling when CTRL is not pressed
        GraphView::wheelEvent(event);
    }
    emit graphMoved();
}

void CutterGraphView::resizeEvent(QResizeEvent *event)
{
    GraphView::resizeEvent(event);
    emit resized();
}

void CutterGraphView::saveCurrentBlock() {}

void CutterGraphView::restoreCurrentBlock() {}

void CutterGraphView::mousePressEvent(QMouseEvent *event)
{
    GraphView::mousePressEvent(event);
    emit graphMoved();
}

void CutterGraphView::mouseMoveEvent(QMouseEvent *event)
{
    GraphView::mouseMoveEvent(event);
    emit graphMoved();
}

void CutterGraphView::exportGraph(QString filePath, GraphExportType exportType,
                                  RzCoreGraphType graphType, RVA address)
{
    bool graphTransparent = Config()->getBitmapTransparentState();
    double graphScaleFactor = Config()->getBitmapExportScaleFactor();
    switch (exportType) {
    case GraphExportType::Png:
        this->saveAsBitmap(filePath, "png", graphScaleFactor, graphTransparent);
        break;
    case GraphExportType::Jpeg:
        this->saveAsBitmap(filePath, "jpg", graphScaleFactor, false);
        break;
    case GraphExportType::Svg:
        this->saveAsSvg(filePath);
        break;

    case GraphExportType::GVDot:
        exportRzTextGraph(filePath, graphType, RZ_CORE_GRAPH_FORMAT_DOT, address);
        break;
    case GraphExportType::RzJson:
        exportRzTextGraph(filePath, graphType, RZ_CORE_GRAPH_FORMAT_JSON, address);
        break;
    case GraphExportType::RzGml:
        exportRzTextGraph(filePath, graphType, RZ_CORE_GRAPH_FORMAT_GML, address);
        break;

    case GraphExportType::GVJson:
        Core()->writeGraphvizGraphToFile(filePath, "json", graphType, address);
        break;
    case GraphExportType::GVGif:
        Core()->writeGraphvizGraphToFile(filePath, "gif", graphType, address);
        break;
    case GraphExportType::GVPng:
        Core()->writeGraphvizGraphToFile(filePath, "png", graphType, address);
        break;
    case GraphExportType::GVJpeg:
        Core()->writeGraphvizGraphToFile(filePath, "jpg", graphType, address);
        break;
    case GraphExportType::GVPostScript:
        Core()->writeGraphvizGraphToFile(filePath, "ps", graphType, address);
        break;
    case GraphExportType::GVSvg:
        Core()->writeGraphvizGraphToFile(filePath, "svg", graphType, address);
        break;
    case GraphExportType::GVPdf:
        Core()->writeGraphvizGraphToFile(filePath, "pdf", graphType, address);
        break;
    }
}

void CutterGraphView::exportRzTextGraph(QString filePath, RzCoreGraphType type,
                                        RzCoreGraphFormat format, RVA address)
{
    char *string = Core()->getTextualGraphAt(type, format, address);
    if (!string) {
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream fileOut(&file);
        fileOut << string;
    } else {
        qWarning() << "Can't open or create file: " << filePath;
    }
    free(string);
}

bool CutterGraphView::graphIsBitamp(CutterGraphView::GraphExportType type)
{
    switch (type) {
    case GraphExportType::Png:
    case GraphExportType::Jpeg:
    case GraphExportType::GVGif:
    case GraphExportType::GVPng:
    case GraphExportType::GVJpeg:
        return true;
    default:
        return false;
    }
}

Q_DECLARE_METATYPE(CutterGraphView::GraphExportType);

void CutterGraphView::showExportGraphDialog(QString defaultName, RzCoreGraphType type, RVA address)
{
    qWarning() << defaultName << " - " << type << " addr " << RzAddressString(address);
    QVector<MultitypeFileSaveDialog::TypeDescription> types = {
        { tr("PNG (*.png)"), "png", QVariant::fromValue(GraphExportType::Png) },
        { tr("JPEG (*.jpg)"), "jpg", QVariant::fromValue(GraphExportType::Jpeg) },
        { tr("SVG (*.svg)"), "svg", QVariant::fromValue(GraphExportType::Svg) }
    };

    types.append({
            { tr("Graphviz dot (*.dot)"), "dot", QVariant::fromValue(GraphExportType::GVDot) },
            { tr("Graph Modelling Language (*.gml)"), "gml",
              QVariant::fromValue(GraphExportType::RzGml) },
            { tr("RZ JSON (*.json)"), "json", QVariant::fromValue(GraphExportType::RzJson) },
    });

    bool hasGraphviz = !QStandardPaths::findExecutable("dot").isEmpty()
            || !QStandardPaths::findExecutable("xdot").isEmpty();
    if (hasGraphviz) {
        types.append({ { tr("Graphviz json (*.json)"), "json",
                         QVariant::fromValue(GraphExportType::GVJson) },
                       { tr("Graphviz gif (*.gif)"), "gif",
                         QVariant::fromValue(GraphExportType::GVGif) },
                       { tr("Graphviz png (*.png)"), "png",
                         QVariant::fromValue(GraphExportType::GVPng) },
                       { tr("Graphviz jpg (*.jpg)"), "jpg",
                         QVariant::fromValue(GraphExportType::GVJpeg) },
                       { tr("Graphviz PostScript (*.ps)"), "ps",
                         QVariant::fromValue(GraphExportType::GVPostScript) },
                       { tr("Graphviz svg (*.svg)"), "svg",
                         QVariant::fromValue(GraphExportType::GVSvg) },
                       { tr("Graphviz pdf (*.pdf)"), "pdf",
                         QVariant::fromValue(GraphExportType::GVPdf) } });
    }

    MultitypeFileSaveDialog dialog(this, tr("Export Graph"));
    dialog.setTypes(types);
    dialog.selectFile(defaultName);
    if (!dialog.exec()) {
        return;
    }

    auto selectedType = dialog.selectedType();
    if (!selectedType.data.canConvert<GraphExportType>()) {
        qWarning() << "Bad selected type, should not happen.";
        return;
    }
    auto exportType = selectedType.data.value<GraphExportType>();

    if (graphIsBitamp(exportType)) {
        uint64_t bitmapSize = uint64_t(width) * uint64_t(height);
        if (bitmapSize > BITMPA_EXPORT_WARNING_SIZE) {
            auto answer =
                    QMessageBox::question(this, tr("Graph Export"),
                                          tr("Do you really want to export %1 x %2 = %3 pixel "
                                             "bitmap image? Consider using different format.")
                                                  .arg(width)
                                                  .arg(height)
                                                  .arg(bitmapSize));
            if (answer != QMessageBox::Yes) {
                return;
            }
        }
    }

    QString filePath = dialog.selectedFiles().first();
    exportGraph(filePath, exportType, type, address);
}
