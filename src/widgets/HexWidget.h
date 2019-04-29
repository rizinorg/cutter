#ifndef HEXWIDGET_H
#define HEXWIDGET_H

#include <QScrollArea>
#include <QTimer>

struct HexCursor {
    HexCursor() { isVisible = false; onAsciiArea = false; }

    bool isVisible; // FIXME: move to HexWidget
    bool onAsciiArea;
    QTimer blinkTimer;
    QRect screenPos;
    uint64_t address;
    QString cachedChar;
    QColor cachedColor;

    void blink() { isVisible = !isVisible; }
    void setBlinkPeriod(int msec) { blinkTimer.setInterval(msec / 2); }
    void startBlinking() { blinkTimer.start(); }
    void stopBlinking() { blinkTimer.stop(); }
};

struct MemoryCache {
    QVector<QByteArray> blocks;
    uint64_t firstBlockAddr;
    int firstBlockOffset;

    const void *dataPtr(int offset);
};

class HexSelection
{
public:
    HexSelection() { m_empty = true; }

    inline void init(uint64_t addr)
    {
        m_empty = true;
        m_init = addr;
    }

    void update(uint64_t addr)
    {
        m_empty = false;
        if (addr > m_init) {
            m_start = m_init;
            m_end = addr - 1;
        } else if (addr < m_init) {
            m_start = addr;
            m_end = m_init - 1;
        } else {
            m_start = m_end = m_init;
            m_empty = true;
        }
    }

    bool intersects(uint64_t start, uint64_t end)
    {
        return !m_empty && !(m_end <= start || m_start >= end);
    }

    bool contains(uint64_t pos) const
    {
        return !m_empty && m_start <= pos && pos <= m_end;
    }

    uint64_t size()
    {
        uint64_t size = 0;
        if (!isEmpty())
            size = m_end - m_start + 1;
        return size;
    }

    inline bool isEmpty() { return m_empty; }
    inline uint64_t start() { return m_start; }
    inline uint64_t end() { return m_end; }

private:
    uint64_t m_init;
    uint64_t m_start;
    uint64_t m_end;
    bool m_empty;
};

class HexWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit HexWidget(QWidget *parent = nullptr);
    ~HexWidget();

    void setFont(const QFont &font);

    enum AddrWidth { AddrWidth32 = 8, AddrWidth64 = 16 };
    enum ItemSize { ItemSizeByte = 1, ItemSizeWord = 2, ItemSizeDword = 4, ItemSizeQword = 8 };
    enum ItemFormat { ItemFormatHex, ItemFormatOct, ItemFormatDec, ItemFormatSignedDec, ItemFormatFloat };

    void setItemSize(int nbytes);
    void setItemFormat(ItemFormat format);
    void setItemEndianess(bool bigEndian);
    void setItemGroupSize(int size);
    void setColumnCount(int columns);

public slots:
    void onSeekChanged(uint64_t addr);
    void updateColors();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);

private slots:
    void showContextMenu(const QPoint &pt);
    void onCursorBlinked();
    void onHexPairsModeEnabled(bool enable);
    void copy();
    void copyAddress();

private:
    void updateItemLength();
    void drawHeader(QPainter &painter);
    void drawCursor(QPainter &painter, bool shadow = false);
    void drawAddrArea(QPainter &painter);
    void drawItemArea(QPainter &painter);
    void drawAsciiArea(QPainter &painter);
    void fillSelectionBackground(QPainter &painter, bool ascii = false);
    void updateMetrics();
    void updateAreasPosition();
    void updateAreasHeight();
    void moveCursor(int offset, bool select = false);
    void setCursorAddr(uint64_t addr, bool select = false);
    void updateCursorMeta();
    void setCursorOnAscii(bool ascii);
    const QColor itemColor(uint8_t byte);
    QVariant readItem(int offset, QColor *color = nullptr);
    QString renderItem(int offset, QColor *color = nullptr);
    QChar renderAscii(int offset, QColor *color = nullptr);
    void updateDataCache();
    uint64_t screenPosToAddr(const QPoint &point) const;
    uint64_t asciiPosToAddr(const QPoint &point) const;
    uint64_t currentAreaPosToAddr(const QPoint &point) const;
    QRect itemRectangle(uint offset);
    QRect asciiRectangle(uint offset);

    inline int itemWidth() const
    {
        return itemCharLen * charWidth;
    }

    inline int itemGroupCharLen() const
    {
        return itemCharLen * itemGroupSize;
    }

    inline int columnExCharLen() const
    {
        return itemGroupCharLen() + columnSpacing;
    }

    inline int itemGroupByteLen() const
    {
        return itemByteLen * itemGroupSize;
    }

    inline int columnWidth() const
    {
        return itemGroupCharLen() * charWidth;
    }

    inline int columnExWidth() const
    {
        return columnExCharLen() * charWidth;
    }

    inline int columnSpacingWidth() const
    {
        return columnSpacing * charWidth;
    }

    inline int itemRowCharLen() const
    {
        return itemColumns * columnExCharLen() - columnSpacing;
    }

    inline int itemRowByteLen() const
    {
        return itemColumns * itemGroupByteLen();
    }

    inline int bytesPerScreen() const
    {
        return itemRowByteLen() * visibleLines;
    }

    inline int itemRowWidth() const
    {
        return itemRowCharLen() * charWidth;
    }

    inline int asciiRowWidth() const
    {
        return itemRowByteLen() * charWidth;
    }

    inline int areaSpacingWidth() const
    {
        return areaSpacing * charWidth;
    }

    inline uint64_t lastVisibleAddr() const
    {
        return (startAddress - 1) + bytesPerScreen();
    }

    const QRect &currentArea() const
    {
        return cursorOnAscii ? asciiArea : itemArea;
    }

    bool cursorEnabled;
    bool cursorOnAscii;
    HexCursor cursor;
    HexCursor shadowCursor;

    HexSelection selection;
    bool updatingSelection;

    QRect addrArea;
    QRect itemArea;
    QRect asciiArea;

    int itemByteLen;
    int itemGroupSize; // Items per group (default: 1)
    int itemColumns;
    int itemCharLen;
    int itemPrefixLen;

    ItemFormat itemFormat;

    bool itemBigEndian;

    int visibleLines;
    uint64_t startAddress;
    int charWidth;
    int byteWidth;
    int lineHeight;
    int addrCharLen;
    int addrAreaWidth;

    bool showHeader;
    bool showAscii;
    bool showExHex;
    bool showExAddr;

    QColor borderColor;
    QColor backgroundColor;
    QColor defColor;
    QColor addrColor;
    QColor b0x00Color;
    QColor b0x7fColor;
    QColor b0xffColor;
    QColor printableColor;

    MemoryCache memCache;

    /* Spacings in characters */
    const int columnSpacing = 1;
    const int areaSpacing = 2;

    const QString hexPrefix = QStringLiteral("0x");

    QList<QAction *> actionsColumnCount;
    QList<QAction *> actionsItemSize;
    QList<QAction *> actionsItemFormat;
    QAction *actionItemBigEndian;
    QAction *actionHexPairs;
    QAction *actionCopy;
    QAction *actionCopyAddress;

    const uint64_t maxIndex = UINT64_MAX;
};

#endif // HEXWIDGET_H
