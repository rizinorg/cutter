#include "HexDiffView.h"

#include <QPainterPath>

HexDiffViewSyncer::HexDiffViewSyncer(CutterDiff *cutterDiff, HexDiffScrollBar *scrollBar,
                                     QObject *parent)
    : QObject(parent), cutterDiff(cutterDiff), scrollBar(scrollBar), addrCharLen(AddrWidth64)
{
    updateScrollBar();
}

DiffItemSize HexDiffViewSyncer::getItemSize()
{
    return itemSize;
}

void HexDiffViewSyncer::setItemSize(DiffItemSize size)
{
    if (itemSize == size) {
        return;
    }
    itemSize = size;
    if (itemSize != ItemSizeByte) {
        groupItems = false;
    }
    emit refreshView();
}

void HexDiffViewSyncer::setItemFormat(DiffItemFormat format)
{
    if (itemFormat == format) {
        return;
    }
    itemFormat = format;
    if (itemFormat != ItemFormatHex) {
        groupItems = false;
    }
    emit refreshView();
}

bool HexDiffViewSyncer::isLittleEndian() const
{
    return littleEndian;
}

void HexDiffViewSyncer::setLittleEndian(bool endian)
{
    littleEndian = endian;
    emit refreshView();
}

ut8 HexDiffViewSyncer::getBytesPerRow() const
{
    return static_cast<ut8>(1u << powBytesPerRow);
}

ut8 HexDiffViewSyncer::getItemsPerRow() const
{
    return getBytesPerRow() / itemSize;
}

void HexDiffViewSyncer::setPowBytesPerRow(ut8 pow)
{
    powBytesPerRow = pow;
    updateScrollBar();
    emit refreshView();
}

void HexDiffViewSyncer::setShiftA(int shift)
{
    shiftA = shift;
    emit refreshView();
}

void HexDiffViewSyncer::setShiftB(int shift)
{
    shiftB = shift;
    emit refreshView();
}

void HexDiffViewSyncer::setGroupItems(bool group)
{
    // Byte pairing is only valid for 1-byte hexadecimal items.
    if (group && (itemSize != ItemSizeByte || itemFormat != ItemFormatHex)) {
        return;
    }
    if (groupItems == group) {
        return;
    }
    groupItems = group;
    emit refreshView();
}

int HexDiffViewSyncer::getItemLen() const
{
    switch (itemFormat) {
    case ItemFormatHex:
        return static_cast<int>(itemSize) * 2
                + ((showExAddr && (itemSize != ItemSizeByte)) ? 2 : 0);

    case ItemFormatOct:
        switch (itemSize) {
        case ItemSizeByte:
            return 3;
        case ItemSizeWord:
            return 6;
        case ItemSizeDword:
            return 11;
        case ItemSizeQword:
            return 22;
        }
        break;

    case ItemFormatDec:
        switch (itemSize) {
        case ItemSizeByte:
            return 3;
        case ItemSizeWord:
            return 5;
        case ItemSizeDword:
            return 10;
        case ItemSizeQword:
            return 20;
        }
        break;

    case ItemFormatSignedDec:
        switch (itemSize) {
        case ItemSizeByte:
            return 4;
        case ItemSizeWord:
            return 6;
        case ItemSizeDword:
            return 11;
        case ItemSizeQword:
            return 20;
        }
        break;

    case ItemFormatFloat:
        switch (itemSize) {
        case ItemSizeDword:
            return 13;
        case ItemSizeQword:
            return 18;
        default:
            return 0;
        }
    }

    return 0;
}

int HexDiffViewSyncer::getGroupedItemLen() const
{
    const int multiplier = getGroupItems() ? 2 : 1;
    return getItemLen() * multiplier;
}

QString HexDiffViewSyncer::renderItem(quint64 byteOffset, bool orig)
{
    const int groupSize = getGroupItems() ? 2 : 1;

    QString result;

    for (int i = 0; i < groupSize; ++i) {
        const int index = byteOffset + i;

        const RVA addr = getAddressAt(index, orig);

        QByteArray data = cutterDiff->ioRead(addr, static_cast<int>(itemSize), orig);

        if (data.size() != static_cast<int>(itemSize)) {
            continue;
        }

        quint64 value = 0;
        if (!isLittleEndian()) {
            for (int j = 0; j < data.size(); ++j) {
                value <<= 8;
                value |= static_cast<ut8>(data[j]);
            }
        } else {
            for (int j = 0; j < data.size(); ++j) {
                value |= static_cast<quint64>(static_cast<ut8>(data[j])) << (j * 8);
            }
        }
        QString rendered;
        switch (itemFormat) {
        case ItemFormatHex: {
            rendered = QString::number(value, 16)
                               .rightJustified(itemSize * 2, '0')
                               .toUpper(); // itemsize for hex because of exAddr consistancy issues
                                           // in itemLen

            if (showExAddr && (itemSize > ItemSizeByte)) {
                rendered.prepend(QStringLiteral("0x"));
            }
            break;
        }

        case ItemFormatOct: {
            rendered = QString::number(value, 8).rightJustified(getItemLen(), '0').toUpper();
            break;
        }

        case ItemFormatDec: {
            rendered = QString::number(value, 10).rightJustified(getItemLen(), '0').toUpper();
            break;
        }

        case ItemFormatSignedDec: {
            switch (itemSize) {
            case ItemSizeByte: {
                rendered = QString("%1").arg(static_cast<qint8>(value), getItemLen(), 10,
                                             QLatin1Char(' '));
                break;
            }

            case ItemSizeWord: {
                rendered = QString("%1").arg(static_cast<qint16>(value), getItemLen(), 10,
                                             QLatin1Char(' '));
                break;
            }

            case ItemSizeDword: {
                rendered = QString("%1").arg(static_cast<qint32>(value), getItemLen(), 10,
                                             QLatin1Char(' '));
                break;
            }

            case ItemSizeQword: {
                rendered = QString("%1").arg(static_cast<qint64>(value), getItemLen(), 10,
                                             QLatin1Char(' '));
                break;
            }
            }
            break;
        }

        case ItemFormatFloat: {
            if (itemSize == ItemSizeDword) {
                float f;
                const quint32 bits = static_cast<quint32>(value);
                memcpy(&f, &bits, sizeof(f));

                rendered = QString::number(f, 'e', 6);
            } else if (itemSize == ItemSizeQword) {
                double d;
                quint64 bits = value;
                memcpy(&d, &bits, sizeof(d));

                rendered = QString::number(d, 'e', 10);
            }
            break;
        }
        }
        result += rendered;
    }
    return result;
}

QString HexDiffViewSyncer::renderAscii(quint64 byteOffset, bool orig)
{
    const RVA addr = getAddressAt(byteOffset, orig);
    const QByteArray data = cutterDiff->ioRead(addr, 1, orig);

    if (data.isEmpty()) {
        return ".";
    }

    const auto byte = static_cast<unsigned char>(data.at(0));

    if (byte < 32 || byte > 126) {
        return ".";
    }

    return QString(QChar::fromLatin1(byte));
}

RVA HexDiffViewSyncer::shifted(RVA addr, qint64 shift) const
{
    if (shift < 0) {
        return addr - qAbs(shift);
    } else {
        return addr + qAbs(shift);
    }
}

RVA HexDiffViewSyncer::getAddressAt(quint64 pos, bool orig) const
{
    if (orig) {
        return shifted(startAddress + pos, shiftA);
    } else {
        return shifted(startAddress + pos, shiftB);
    }
}

bool HexDiffViewSyncer::diffItem(quint64 byteOffset, bool orig)
{
    const RVA addrA = getAddressAt(byteOffset, orig);
    const RVA addrB = getAddressAt(byteOffset, !orig);

    const QByteArray dataA = cutterDiff->ioRead(addrA, itemSize, orig);
    const QByteArray dataB = cutterDiff->ioRead(addrB, itemSize, !orig);

    return dataA != dataB;
}

bool HexDiffViewSyncer::diffByte(quint64 byteOffset, bool orig)
{
    const RVA addrA = getAddressAt(byteOffset, orig);
    const RVA addrB = getAddressAt(byteOffset, !orig);

    const QByteArray dataA = cutterDiff->ioRead(addrA, 1, orig);
    const QByteArray dataB = cutterDiff->ioRead(addrB, 1, !orig);

    return dataA != dataB;
}

void HexDiffViewSyncer::beginSelection(quint64 offset, bool orig)
{
    cursorOwnedByA = orig;
    selection.posA = getAddressAt(offset, orig);
    selection.posB = getAddressAt(offset, orig);
    emit selectionChanged();
}

void HexDiffViewSyncer::updateSelection(quint64 offset, bool orig)
{
    selection.posB = getAddressAt(offset, orig);
    emit selectionChanged();
}

void HexDiffViewSyncer::clearSelection()
{
    selection.clear();
    emit selectionChanged();
}

HexDiffSelection HexDiffViewSyncer::getSelectionOffsets(quint64 lastOffset, bool orig)
{
    HexDiffSelection result;

    if (!selection.isValid()) {
        return result;
    }

    const RVA baseAddress = getAddressAt(0, cursorOwnedByA);
    const RVA lastAddress = getAddressAt(lastOffset, orig);

    const RVA startAddress = qMax(baseAddress, selection.start());
    const RVA endAddress = selection.end();

    result.posA = startAddress - baseAddress;
    result.posB = qMin(endAddress - baseAddress, lastAddress - baseAddress);

    return result;
}

void HexDiffViewSyncer::onScrollBarAddressChanged(RVA value)
{
    const RVA startAddressOffset = startAddress % getBytesPerRow();
    const RVA newBase = value / getBytesPerRow();
    startAddress = newBase * getBytesPerRow() + startAddressOffset;
    emit refreshView();
}

void HexDiffViewSyncer::scrollTo(RVA addr)
{
    scrollBar->setAddress(addr);
}
void HexDiffViewSyncer::scrollBy(int lineCount)
{
    RVA addr;
    if (lineCount < 0) {
        addr = scrollBar->address() - std::abs(lineCount) * getBytesPerRow();
    } else {
        addr = scrollBar->address() + std::abs(lineCount) * getBytesPerRow();
    }
    scrollBar->setAddress(addr);
}

void HexDiffViewSyncer::updateScrollBar()
{
    if (scrollBar == nullptr) {
        return;
    }
    const std::pair<RVA, RVA> rangeA = cutterDiff->addressRange(true);
    const std::pair<RVA, RVA> rangeB = cutterDiff->addressRange(false);
    const RVA begin = qMin(rangeA.first, rangeB.first);
    const RVA end = qMax(rangeA.second, rangeB.second);
    scrollBar->setAddressRange(begin, end);
    scrollBar->setAddress(rangeA.first);
    scrollBar->setAddressSingleStep(getBytesPerRow());
    scrollBar->setAddressPageStep(16 * getBytesPerRow());
}

quint64 HexDiffViewSyncer::getCursorOffset()
{
    const qint64 shift = cursorOwnedByA ? shiftA : shiftB;
    const RVA base = shifted(startAddress, shift);

    if (cursorAddr < base) {
        return UINT64_MAX;
    }

    return cursorAddr - base;
}

void HexDiffViewSyncer::setCursor(quint64 offset, bool orig)
{
    const RVA newCursorAddr = getAddressAt(offset, orig);
    if (newCursorAddr == cursorAddr && cursorOwnedByA == orig) {
        return;
    }
    cursorOwnedByA = orig;
    cursorAddr = newCursorAddr;
    emit refreshView();
}

HexDiffView::HexDiffView(HexDiffViewSyncer *syncer, bool orig, QWidget *parent)
    : QScrollArea(parent), syncer(syncer), orig(orig), autoScrollTimer(new QTimer(this))
{
    autoScrollTimer->setInterval(30);
    connect(autoScrollTimer, &QTimer::timeout, this, [this]() { this->autoScroll(); });
    connect(Config(), &Configuration::colorsUpdated, this, &HexDiffView::updateColors);
    connect(Config(), &Configuration::fontsUpdated, this, &HexDiffView::setMonospaceFont);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int) { viewport()->update(); });
    connect(syncer, &HexDiffViewSyncer::selectionChanged, this, &HexDiffView::updateViewport);
    connect(syncer, &HexDiffViewSyncer::refreshView, this, &HexDiffView::updateViewport);

    updateColors();
    setMonospaceFont();
    updateViewport();
    updateAreaGeometry();
}

void HexDiffView::updateViewport()
{
    updateScrollbarWidth();
    updateAreaGeometry();
    viewport()->update();
}

void HexDiffView::updateColors()
{
    backgroundColor = Config()->getColor("gui.background");
    borderColor = Config()->getColor("gui.border");
    defColor = Config()->getColor("btext");
    diffColor = Config()->getColor("graph.diff.unmatch");
    addrColor = Config()->getColor("func_var_addr");
    updateViewport();
}

void HexDiffView::setMonospaceFont()
{
    QScrollArea::setFont(Config()->getFont());
    if (!(font().styleHint() & QFont::Monospace)) {
        /* FIXME: Use default monospace font
        setFont(XXX); */
    }
    monospaceFont = font().resolve(this->font());
    const QFontMetricsF fontMetrics(this->monospaceFont);
    lineHeight = fontMetrics.height();

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    charWidth = fontMetrics.width('A');
#else
    charWidth = fontMetrics.horizontalAdvance('A');
#endif
    updateViewport();
}

void HexDiffView::updateAreaGeometry()
{
    const qreal yOffset = syncer->getHeaderEnabled() ? (1 + rowGap) * lineHeight : 0;

    addrArea.setTopLeft(QPoint(0, yOffset));
    addrArea.setWidth(syncer->getAddrLen() * charWidth);

    itemArea.setTopLeft(QPoint(addrArea.right() + getAreaSpacing(), addrArea.top()));
    itemArea.setWidth(((syncer->getGroupedItemLen() + columnGap) * syncer->getGroupedItemsPerRow())
                      * charWidth);
    asciiArea.setTopLeft(QPoint(itemArea.right() + getAreaSpacing(), yOffset));
    asciiArea.setWidth(((1 + columnGap) * syncer->getBytesPerRow()) * charWidth);

    const qreal areaHeight = viewport()->rect().height() - yOffset;
    addrArea.setHeight(areaHeight);
    itemArea.setHeight(areaHeight);
    asciiArea.setHeight(areaHeight);

    visibleLines = itemArea.height() / (lineHeight + rowGap * lineHeight);
}

void HexDiffView::resizeEvent(QResizeEvent *event)
{
    updateViewport();
    QScrollArea::resizeEvent(event);
}

void HexDiffView::updateScrollbarWidth()
{
    int max = (syncer->getShowAscii() ? asciiArea.right() : itemArea.right()) - viewport()->width();

    if (max < 0) {
        max = 0;
    } else {
        max += charWidth;
    }

    horizontalScrollBar()->setMaximum(max);
    horizontalScrollBar()->setSingleStep(charWidth);
}

void HexDiffView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.setBrush(backgroundColor);
    painter.drawRect(event->rect());
    painter.setFont(this->monospaceFont);

    const int xOffset = horizontalScrollBar()->value();

    if (xOffset > 0) {
        painter.translate(QPoint(-xOffset, 0));
    }

    drawHeaderArea(painter);
    drawAddressArea(painter);
    drawItemArea(painter);
    drawAsciiArea(painter);
    drawItemCursor(painter);
    drawAsciiCursor(painter);
}

void HexDiffView::drawHeaderArea(QPainter &painter)
{
    const qreal itemWidth = syncer->getGroupedItemLen() * charWidth;
    const qreal itemHeight = lineHeight + rowGap * lineHeight;
    QRectF rect(itemArea.left(), 0, itemWidth, itemHeight);
    painter.setPen(addrColor);
    if (syncer->getGroupItems()) {
        for (int i = 0; i < syncer->getItemsPerRow(); i += 2) {
            rect.translate(getColumnSpacing() / 2, 0);
            painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight,
                             QString::number(i, 16) + " " + QString::number(i + 1, 16).toUpper());
            rect.translate(itemWidth + getColumnSpacing() / 2, 0);
        }
    } else {
        for (int i = 0; i < syncer->getItemsPerRow(); i++) {
            rect.translate(getColumnSpacing() / 2, 0);
            painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight,
                             QString::number(i, 16).toUpper());
            rect.translate(itemWidth + getColumnSpacing() / 2, 0);
        }
    }

    const qreal asciiWidth = charWidth;
    rect.setLeft(asciiArea.left());
    rect.setWidth(asciiWidth);
    if (syncer->getShowAscii()) {
        for (int i = 0; i < syncer->getBytesPerRow(); i++) {
            rect.translate(getColumnSpacing() / 2, 0);
            painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight,
                             QString::number(i, 16) + " " + QString::number(i, 16).toUpper());
            rect.translate(asciiWidth + getColumnSpacing() / 2, 0);
        }
    }
}

void HexDiffView::drawAddressItem(QPainter &painter, const QRectF &rect, RVA addr)
{
    painter.setPen(addrColor);
    QString text = syncer->getShowExAddr() ? "0x" : "";
    text += QString::number(addr, 16).rightJustified(syncer->getAddrCharLen(), '0');
    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, text);
}

void HexDiffView::drawAddressArea(QPainter &painter)
{
    const qreal addrItemWidth = syncer->getAddrLen() * charWidth;
    const qreal addrItemHeight = lineHeight + rowGap * lineHeight;
    painter.setBrush(Qt::white);
    QRectF rect(addrArea.left(), addrArea.top(), addrItemWidth, addrItemHeight);
    for (int i = 0; i < visibleLines; i++) {
        drawAddressItem(painter, rect, syncer->getAddressAt(syncer->getBytesPerRow() * i, orig));
        rect.translate(0, addrItemHeight);
    }
}

void HexDiffView::drawItem(QPainter &painter, QRectF rect, quint64 byteOffset,
                           const QPolygonF &clip)
{
    rect.setWidth(rect.width() - getColumnSpacing() / 2);
    painter.save();
    painter.setPen(defColor);
    if (syncer->diffItem(byteOffset, orig)) {
        painter.setPen(diffColor);
    }
    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, syncer->renderItem(byteOffset, orig));
    if (syncer->getSelectionOffsets(lastVisibleOffset(), orig).contains(byteOffset)
        || syncer->getSelectionOffsets(lastVisibleOffset(), orig)
                   .contains(byteOffset + syncer->getGroupedItemSize())) {
        QPainterPath path;
        path.addPolygon(clip);

        painter.setClipPath(path, Qt::IntersectClip);
        painter.setPen(palette().color(QPalette::HighlightedText));
        painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter,
                         syncer->renderItem(byteOffset, orig));
    }
    painter.restore();
}

QPolygonF HexDiffView::getItemSelectionPolygon()
{
    const HexDiffSelection selection = syncer->getSelectionOffsets(lastVisibleOffset(), orig);

    if (!selection.isValid()) {
        return QPolygonF();
    }

    if (selection.end() == 0) {
        return QPolygonF();
    }

    const quint64 itemSize = syncer->getGroupedItemSize();
    const quint64 bytesPerRow = syncer->getBytesPerRow();
    const quint64 start = selection.start();
    const quint64 end = selection.end();

    const quint64 last = end - 1;
    const QRectF startRect = getItemRectAtOffset(start);
    const QRectF stopRect = getItemRectAtOffset(last);

    if (startRect.isEmpty() || stopRect.isEmpty()) {
        return QPolygonF();
    }

    const quint64 inItemStartOffset = start % itemSize;
    const quint64 inItemEndOffset = last % itemSize;

    const qreal addWidthStart = startRect.width() * (qreal(inItemStartOffset) / qreal(itemSize));

    const qreal addWidthEnd =
            stopRect.width() * (qreal(itemSize - inItemEndOffset - 1) / qreal(itemSize));

    const qreal startX = startRect.left() + addWidthStart;
    const qreal endX = stopRect.right() - addWidthEnd;

    const quint64 startRow = start / bytesPerRow;
    const quint64 endRow = last / bytesPerRow;
    const bool multiRow = startRow != endRow;

    const qreal tail = charWidth * 0.5;

    QPolygonF poly;

    if (inItemStartOffset != 0) {
        poly << QPointF(startX + tail, startRect.top()) << QPointF(startX, startRect.center().y())
             << QPointF(startX + tail, startRect.bottom());
    } else {
        poly << QPointF(startX, startRect.top()) << QPointF(startX, startRect.bottom());
    }

    if (multiRow) {
        poly << QPointF(itemArea.left(), startRect.bottom())
             << QPointF(itemArea.left(), stopRect.bottom());
    }

    if (inItemEndOffset != itemSize - 1) {
        poly << QPointF(endX - tail, stopRect.bottom()) << QPointF(endX, stopRect.center().y())
             << QPointF(endX - tail, stopRect.top());
    } else {
        poly << QPointF(endX, stopRect.bottom()) << QPointF(endX, stopRect.top());
    }

    if (multiRow) {
        poly << QPointF(itemArea.right(), stopRect.top())
             << QPointF(itemArea.right(), startRect.top());
    }
    return poly;
}

void HexDiffView::drawItemSelectionBackground(QPainter &painter, const QPolygonF &poly)
{
    if (poly.isEmpty()) {
        return;
    }

    painter.save();

    if (cursorArea == ItemArea) {
        QColor backgroundColor = palette().color(QPalette::Highlight);
        backgroundColor.setAlpha(100);
        painter.setBrush(backgroundColor);
    } else {
        painter.setBrush(Qt::NoBrush);
    }

    painter.setPen(palette().color(QPalette::Highlight));
    painter.drawPolygon(poly);

    painter.restore();
}

void HexDiffView::drawAsciiSelectionBackground(QPainter &painter)
{
    const HexDiffSelection selection = syncer->getSelectionOffsets(lastVisibleOffset(), orig);

    if (!selection.isValid()) {
        return;
    }

    if (selection.end() == 0) {
        return;
    }

    const quint64 bytesPerRow = syncer->getBytesPerRow();

    const quint64 start = selection.start();
    const quint64 end = selection.end();
    const quint64 last = end - 1;

    const QRectF startRect = getAsciiItemRectAtOffset(start);
    const QRectF stopRect = getAsciiItemRectAtOffset(last);

    if (startRect.isEmpty() || stopRect.isEmpty()) {
        return;
    }

    const qreal startX = startRect.left();
    const qreal endX = stopRect.right();

    const quint64 startRow = start / bytesPerRow;
    const quint64 endRow = last / bytesPerRow;

    const bool multiRow = startRow != endRow;

    const QPointF p1(startX, startRect.top());
    const QPointF p2(startX, startRect.bottom());

    const QPointF p3(asciiArea.left(), startRect.bottom());
    const QPointF p4(asciiArea.left(), stopRect.bottom());

    const QPointF p5(endX, stopRect.bottom());
    const QPointF p6(endX, stopRect.top());

    const QPointF p7(asciiArea.right(), stopRect.top());
    const QPointF p8(asciiArea.right(), startRect.top());

    QPolygonF poly;

    poly << p1 << p2;

    if (multiRow) {
        poly << p3 << p4;
    }

    poly << p5 << p6;

    if (multiRow) {
        poly << p7 << p8;
    }

    painter.save();
    if (cursorArea == AsciiArea) {
        QColor backgroundColor = palette().color(QPalette::Highlight);

        backgroundColor.setAlpha(100);
        painter.setBrush(backgroundColor);
    } else {
        painter.setBrush(Qt::NoBrush);
    }
    painter.setPen(palette().color(QPalette::Highlight));
    painter.drawPolygon(poly);
    painter.restore();
}

void HexDiffView::drawItemArea(QPainter &painter)
{
    const QPolygonF selectionPoly = getItemSelectionPolygon();
    drawItemSelectionBackground(painter, selectionPoly);
    const qreal itemWidth = syncer->getGroupedItemLen() * charWidth + getColumnSpacing();
    const qreal itemHeight = lineHeight + getRowSpacing();
    const int itemsPerRow = syncer->getGroupedItemsPerRow();
    QRectF rect(itemArea.left(), itemArea.top(), itemWidth, itemHeight);
    for (int i = 0; i < visibleLines; ++i) {
        rect.moveTo(itemArea.left(), itemArea.top() + i * itemHeight);
        for (int j = 0; j < itemsPerRow; ++j) {
            const quint64 byteOffset =
                    syncer->getBytesPerRow() * i + syncer->getGroupedItemSize() * j;
            drawItem(painter, rect, byteOffset, selectionPoly);
            rect.translate(itemWidth, 0);
        }
    }
    painter.setPen(borderColor);
    const qreal vLineOffset = itemArea.left() - getAreaSpacing() / 2;
    painter.drawLine(QLineF(vLineOffset, 0, vLineOffset, viewport()->height()));
}

void HexDiffView::drawAsciiItem(QPainter &painter, QRectF rect, quint64 byteOffset)
{
    rect.setWidth(rect.width() - getColumnSpacing() / 2);
    painter.setPen(defColor);
    if (syncer->diffByte(byteOffset, orig)) {
        painter.setPen(diffColor);
    }
    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter,
                     syncer->renderAscii(byteOffset, orig));
}

void HexDiffView::drawAsciiArea(QPainter &painter)
{
    drawAsciiSelectionBackground(painter);
    const qreal asciiItemWidth = charWidth + getColumnSpacing();
    const qreal asciiItemHeight = lineHeight + getRowSpacing();
    QRectF rect(asciiArea.left(), asciiArea.top(), asciiItemWidth, asciiItemHeight);
    for (int i = 0; i < visibleLines; ++i) {
        rect.moveTo(asciiArea.left(), asciiArea.top() + i * asciiItemHeight);
        for (int j = 0; j < syncer->getBytesPerRow(); ++j) {

            const quint64 byteOffset = syncer->getBytesPerRow() * i + j;

            drawAsciiItem(painter, rect, byteOffset);
            rect.translate(asciiItemWidth, 0);
        }
    }

    painter.setPen(borderColor);
    const qreal vLineOffset = asciiArea.left() - getAreaSpacing() / 2;
    painter.drawLine(QLineF(vLineOffset, 0, vLineOffset, viewport()->height()));
}

void HexDiffView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QScrollArea::mousePressEvent(event);
        return;
    }
    QPointF pos = event->position();
    pos.rx() += horizontalScrollBar()->value();

    selecting = false;

    syncer->setCursor(getItemOffset(pos), orig);
    if (itemArea.contains(pos)) {
        cursorArea = ItemArea;
        selecting = true;
        syncer->beginSelection(getItemOffset(pos), orig);
    } else if (asciiArea.contains(pos)) {
        cursorArea = AsciiArea;
        selecting = true;
        syncer->beginSelection(getAsciiOffset(pos), orig);
    }
    QScrollArea::mousePressEvent(event);
}
void HexDiffView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF relPos = event->position();
    relPos.rx() += horizontalScrollBar()->value();

    if (itemArea.contains(relPos)) {
        setCursor(Qt::IBeamCursor);
    } else if (asciiArea.contains(relPos)) {
        setCursor(Qt::IBeamCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    if (selecting) {
        if (cursorArea == ItemArea) {
            syncer->updateSelection(getItemOffset(relPos), orig);
        }
        if (cursorArea == AsciiArea) {
            syncer->updateSelection(getAsciiOffset(relPos), orig);
        }
    }
    // autoScroll
    bool outside = !viewport()->rect().contains(event->pos());
    if (selecting && outside) {
        if (!autoScrollTimer->isActive()) {
            autoScrollTimer->start();
        }
    } else {
        autoScrollTimer->stop();
    }
    QScrollArea::mouseMoveEvent(event);
}

void HexDiffView::mouseReleaseEvent(QMouseEvent *event)
{
    QPointF relPos = event->position();
    relPos.rx() += horizontalScrollBar()->value();
    selecting = false;
    autoScrollTimer->stop();
    QScrollArea::mouseReleaseEvent(event);
}

QRectF HexDiffView::getItemRectAtOffset(quint64 offset)
{
    if (!isOffsetVisible(offset)) {
        return QRectF();
    }

    const quint64 itemSize = syncer->getGroupedItemSize();
    const quint64 bytesPerRow = syncer->getBytesPerRow();

    const quint64 itemIndex = (offset % bytesPerRow) / itemSize;
    const quint64 rowIndex = offset / bytesPerRow;

    const qreal itemWidth = syncer->getGroupedItemLen() * charWidth + getColumnSpacing();

    const qreal itemHeight = lineHeight + getRowSpacing();

    return QRectF(itemArea.left() + itemIndex * itemWidth, itemArea.top() + rowIndex * itemHeight,
                  itemWidth, itemHeight);
}

QRectF HexDiffView::getAsciiItemRectAtOffset(quint64 offset)
{
    if (!isOffsetVisible(offset)) {
        return QRectF();
    }

    const quint64 bytesPerRow = syncer->getBytesPerRow();

    const quint64 byteIndex = offset % bytesPerRow;
    const quint64 rowIndex = offset / bytesPerRow;

    const qreal itemWidth = charWidth + getColumnSpacing();

    const qreal itemHeight = lineHeight + getRowSpacing();

    return QRectF(asciiArea.left() + byteIndex * itemWidth, asciiArea.top() + rowIndex * itemHeight,
                  itemWidth, itemHeight);
}

quint64 HexDiffView::getItemOffset(QPointF pos)
{
    pos.setX(std::clamp(pos.x(), itemArea.left(), itemArea.right() + getColumnSpacing()));
    pos.setY(std::clamp(pos.y(), itemArea.top(), itemArea.bottom()));

    const QPointF relativePos = pos - itemArea.topLeft();

    const qreal itemWidth = syncer->getGroupedItemLen() * charWidth;

    const quint64 row = relativePos.y() / (lineHeight + getRowSpacing());

    const quint64 itemIndex = relativePos.x() / (itemWidth + getColumnSpacing());

    return row * syncer->getBytesPerRow() + itemIndex * syncer->getGroupedItemSize();
}

quint64 HexDiffView::getAsciiOffset(QPointF pos)
{
    pos.setX(std::clamp(pos.x(), asciiArea.left(), asciiArea.right() + getColumnSpacing()));
    pos.setY(std::clamp(pos.y(), asciiArea.top(), asciiArea.bottom() + getColumnSpacing()));

    const QPointF relativePos = pos - asciiArea.topLeft();

    const qreal itemWidth = charWidth;

    const quint64 row = relativePos.y() / (lineHeight + getRowSpacing());

    const quint64 itemIndex = relativePos.x() / (itemWidth + getColumnSpacing());

    return row * syncer->getBytesPerRow() + itemIndex;
}

// Context Menu

void HexDiffView::contextMenuEvent(QContextMenuEvent *event)
{
    showContextMenu(event->pos());
}

void HexDiffView::showContextMenu(const QPoint &pos)
{
    QMenu menu(this);

    QMenu *formatMenu = menu.addMenu(tr("Item Format"));

    auto *formatGroup = new QActionGroup(formatMenu);
    formatGroup->setExclusive(true);

    QAction *hexAction = formatMenu->addAction(tr("Hex"));
    QAction *octAction = formatMenu->addAction(tr("Octal"));
    QAction *decAction = formatMenu->addAction(tr("Decimal"));
    QAction *signedDecAction = formatMenu->addAction(tr("Signed Decimal"));
    QAction *floatAction = formatMenu->addAction(tr("Float"));

    hexAction->setCheckable(true);
    octAction->setCheckable(true);
    decAction->setCheckable(true);
    signedDecAction->setCheckable(true);
    floatAction->setCheckable(true);

    formatGroup->addAction(hexAction);
    formatGroup->addAction(octAction);
    formatGroup->addAction(decAction);
    formatGroup->addAction(signedDecAction);
    formatGroup->addAction(floatAction);

    switch (syncer->getItemFormat()) {
    case ItemFormatHex:
        hexAction->setChecked(true);
        break;

    case ItemFormatOct:
        octAction->setChecked(true);
        break;

    case ItemFormatDec:
        decAction->setChecked(true);
        break;

    case ItemFormatSignedDec:
        signedDecAction->setChecked(true);
        break;

    case ItemFormatFloat:
        floatAction->setChecked(true);
        break;
    }

    connect(hexAction, &QAction::triggered, this,
            [this]() { syncer->setItemFormat(ItemFormatHex); });

    connect(octAction, &QAction::triggered, this,
            [this]() { syncer->setItemFormat(ItemFormatOct); });

    connect(decAction, &QAction::triggered, this,
            [this]() { syncer->setItemFormat(ItemFormatDec); });

    connect(signedDecAction, &QAction::triggered, this,
            [this]() { syncer->setItemFormat(ItemFormatSignedDec); });

    connect(floatAction, &QAction::triggered, this,
            [this]() { syncer->setItemFormat(ItemFormatFloat); });

    QMenu *sizeMenu = menu.addMenu(tr("Item Byte Length"));

    auto *sizeGroup = new QActionGroup(sizeMenu);
    sizeGroup->setExclusive(true);

    QAction *byteAction = sizeMenu->addAction(tr("1 Byte"));
    QAction *wordAction = sizeMenu->addAction(tr("2 Bytes"));
    QAction *dwordAction = sizeMenu->addAction(tr("4 Bytes"));
    QAction *qwordAction = sizeMenu->addAction(tr("8 Bytes"));

    byteAction->setCheckable(true);
    wordAction->setCheckable(true);
    dwordAction->setCheckable(true);
    qwordAction->setCheckable(true);

    sizeGroup->addAction(byteAction);
    sizeGroup->addAction(wordAction);
    sizeGroup->addAction(dwordAction);
    sizeGroup->addAction(qwordAction);

    switch (syncer->getItemSize()) {
    case ItemSizeByte:
        byteAction->setChecked(true);
        break;

    case ItemSizeWord:
        wordAction->setChecked(true);
        break;

    case ItemSizeDword:
        dwordAction->setChecked(true);
        break;

    case ItemSizeQword:
        qwordAction->setChecked(true);
        break;
    }

    // Float only supports 4 and 8 byte values.
    const bool floatFormat = syncer->getItemFormat() == ItemFormatFloat;

    byteAction->setEnabled(!floatFormat);
    wordAction->setEnabled(!floatFormat);
    floatAction->setEnabled(syncer->getItemSize() > ItemSizeWord);

    connect(byteAction, &QAction::triggered, this, [this]() { syncer->setItemSize(ItemSizeByte); });

    connect(wordAction, &QAction::triggered, this, [this]() { syncer->setItemSize(ItemSizeWord); });

    connect(dwordAction, &QAction::triggered, this,
            [this]() { syncer->setItemSize(ItemSizeDword); });

    connect(qwordAction, &QAction::triggered, this,
            [this]() { syncer->setItemSize(ItemSizeQword); });

    QAction *pairAction = menu.addAction(tr("Byte Pair"));
    pairAction->setCheckable(true);

    const bool pairAllowed =
            syncer->getItemSize() == ItemSizeByte && syncer->getItemFormat() == ItemFormatHex;

    pairAction->setEnabled(pairAllowed);
    pairAction->setChecked(pairAllowed && syncer->getGroupItems());

    connect(pairAction, &QAction::toggled, this,
            [this](bool enabled) { syncer->setGroupItems(enabled); });

    QAction *littleEndian = menu.addAction(tr("Little Endian"));
    littleEndian->setCheckable(true);
    littleEndian->setChecked(syncer->isLittleEndian());
    connect(littleEndian, &QAction::triggered, this,
            [this]() { syncer->setLittleEndian(!syncer->isLittleEndian()); });

    // Row size byte menu
    QMenu *rowSizeMenu = menu.addMenu(tr("Row Size"));
    auto *rowSizeActionGroup = new QActionGroup(rowSizeMenu);
    rowSizeActionGroup->setExclusive(true);
    for (int i = 0; i < 6; i++) {
        const int rowSize = std::pow(2, i);
        QAction *rowSizeAction = rowSizeMenu->addAction(QString::number(rowSize));
        rowSizeActionGroup->addAction(rowSizeAction);
        rowSizeAction->setCheckable(true);
        rowSizeAction->setChecked(rowSize == syncer->getBytesPerRow());
        connect(rowSizeAction, &QAction::toggled, this,
                [this, i]() { syncer->setPowBytesPerRow(i); });
    }
    // add auto Powers of 2

    menu.exec(mapToGlobal(pos));
}

void HexDiffView::wheelEvent(QWheelEvent *event)
{
    accumScrollWheelDeltaY += event->angleDelta().y();
    // Delta is reported in 1/8 of a degree
    // eg. 120 units * 1/8 = 15 degrees
    // Typical scroll speed is 1 line per 5 degrees
    const int lineDelta = 5 * 8;
    if (accumScrollWheelDeltaY >= lineDelta || accumScrollWheelDeltaY <= -lineDelta) {
        const int lineCount = accumScrollWheelDeltaY / lineDelta;
        accumScrollWheelDeltaY -= lineDelta * lineCount;
        syncer->scrollBy(lineCount);
    }
    QScrollArea::wheelEvent(event);
}

void HexDiffView::drawItemCursor(QPainter &painter)
{
    const quint64 cursorOffset = syncer->getCursorOffset();
    if (cursorOffset > lastVisibleOffset()) {
        return;
    }
    const QRectF rect = getItemRectAtOffset(cursorOffset);
    painter.setBrush(backgroundColor); // erase previous cursor
    painter.setPen(backgroundColor);
    painter.drawRect(rect);
    drawItem(painter, rect, cursorOffset);
    QPen pen;
    pen.setColor(palette().color(QPalette::HighlightedText));
    if (cursorArea != ItemArea) {
        pen.setStyle(Qt::DashDotDotLine);
    }
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect);
}

void HexDiffView::drawAsciiCursor(QPainter &painter)
{
    const quint64 cursorOffset = syncer->getCursorOffset();
    if (cursorOffset > lastVisibleOffset()) {
        return;
    }
    const QRectF rect = getAsciiItemRectAtOffset(cursorOffset);
    painter.setBrush(backgroundColor); // erase previous cursor
    painter.setPen(backgroundColor);
    painter.drawRect(rect);
    drawAsciiItem(painter, rect, cursorOffset);
    QPen pen;
    pen.setColor(palette().color(QPalette::HighlightedText));
    if (cursorArea != AsciiArea) {
        pen.setStyle(Qt::DashDotDotLine);
    }
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect);
}

void HexDiffView::autoScroll()
{
    QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
    enum MouseDir : ut8 {
        MouseLeft = 1u,
        MouseRight = 2u,
        MouseTop = 4u,
        MouseBottom = 8u
        // 5, 9, 6, 10
    };

    ut8 mouseDir = 0;

    if (pos.x() < viewport()->rect().left()) {
        mouseDir |= MouseLeft;
    } else if (pos.x() > viewport()->rect().right()) {
        mouseDir |= MouseRight;
    }

    if (pos.y() < viewport()->rect().top()) {
        mouseDir |= MouseTop;
    } else if (pos.y() > viewport()->rect().bottom()) {
        mouseDir |= MouseBottom;
    }
    const QRect viewportRect = viewport()->rect();
    QRect area = cursorArea == ItemArea ? itemArea.toRect() : asciiArea.toRect();
    area.translate(horizontalScrollBar()->value(), 0);

    if ((mouseDir & MouseLeft) && (viewportRect.left() > area.left())) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - 1);
    }
    if ((mouseDir & MouseRight) && (viewportRect.right() < area.right())) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + 1);
    }

    if (mouseDir & MouseBottom) {
        syncer->scrollBy(1);
    } else if (mouseDir & MouseTop) {
        syncer->scrollBy(-1);
    }
}