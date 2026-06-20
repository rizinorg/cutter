#ifndef HEXDIFF_H
#define HEXDIFF_H

#include "Cutter.h"
#include "common/IOModesController.h"
#include "dialogs/HexdumpRangeDialog.h"
#include "widgets/HexWidget.h"
// #include "tools/bindiff/HexDiffWidget.h"

#include <QMenu>
#include <QScrollArea>
#include <QTimer>

#include <CutterDiff.h>
#include <memory>

class MemoryDiffData
{
public:
    MemoryDiffData(CutterDiff *cutterDiff, bool orig) : cutterDiff(cutterDiff), orig(orig) {}
    ~MemoryDiffData() {}
    static constexpr size_t blockSize = 4096;

    void fetch(uint64_t address, int length)
    {
        // FIXME: reuse data if possible
        // Will be fixing this since for diffing the core files needs efficient cache management
        const uint64_t blockSize = 0x1000ULL;
        const uint64_t alignedAddr = address & ~(blockSize - 1);
        const int offset = address - alignedAddr;
        int len = (offset + length + (blockSize - 1)) & ~(blockSize - 1);
        mFirstBlockAddr = alignedAddr;
        mLastValidAddr = length ? alignedAddr + len - 1 : 0;
        if (mLastValidAddr < mFirstBlockAddr) {
            mLastValidAddr = -1;
            len = mLastValidAddr - mFirstBlockAddr + 1;
        }
        mBlocks.clear();
        uint64_t addr = alignedAddr;
        for (ut64 i = 0; i < len / blockSize; ++i, addr += blockSize) {
            mBlocks.append(cutterDiff->ioRead(addr, blockSize, orig));
        }
    }

    bool copy(void *out, uint64_t addr, size_t len)
    {
        if (addr < mFirstBlockAddr
            || addr > mLastValidAddr
            /* do not merge with previous check to handle overflows */
            || (mLastValidAddr - addr + 1) < len || mBlocks.isEmpty()) {
            memset(out, 0xff, len);
            return false;
        }

        const int totalOffset = addr - mFirstBlockAddr;
        const int blockId = totalOffset / blockSize;
        const int blockOffset = totalOffset % blockSize;
        const size_t firstPart = blockSize - blockOffset;
        if (firstPart >= len) {
            memcpy(out, mBlocks.at(blockId).constData() + blockOffset, len);
        } else {
            memcpy(out, mBlocks.at(blockId).constData() + blockOffset, firstPart);
            memcpy(static_cast<char *>(out) + firstPart, mBlocks.at(blockId + 1).constData(),
                   len - firstPart);
        }
        return true;
    }

    bool write(const uint8_t, uint64_t, size_t) {}

    uint64_t maxIndex() { return std::numeric_limits<uint64_t>::max(); }

    uint64_t minIndex() { return mFirstBlockAddr; }

private:
    bool orig = true;
    CutterDiff *cutterDiff;
    QVector<QByteArray> mBlocks;
    uint64_t mFirstBlockAddr = 0;
    uint64_t mLastValidAddr = 0;
};

// Defining DiffFile Struct for encapsulation
enum DiffFile : ut8 { A, B };

enum DiffArea : ut8 {
    AsciiA,
    AsciiB,
    ItemA,
    ItemB
}; // used modulo and comparisons for getting areas for cursorArea and area in mouse press event
   // both need to be rewritten with a nullarea or no area pointer

class DiffFileContext
{
public:
    DiffFile file;
    uint64_t startAddress;
    std::unique_ptr<MemoryDiffData> data;
    QRectF addrArea;
    QRectF itemArea;
    QRectF asciiArea;
};

/**
 * @brief Tracks memory addresses while preventing 64-bit overflow
 */
struct BasicDiffCursor
{
    uint64_t address;
    bool pastEnd;
    explicit BasicDiffCursor(uint64_t pos) : address(pos), pastEnd(false) {}
    BasicDiffCursor() : address(0), pastEnd(false) {}
    BasicDiffCursor &operator+=(int64_t offset)
    {
        if (offset < 0 && uint64_t(-offset) > address) {
            address = 0;
            pastEnd = false;
        } else if (offset > 0 && uint64_t(offset) > (UINT64_MAX - address)) {
            address = UINT64_MAX;
            pastEnd = true;
        } else {
            address += uint64_t(offset);
            pastEnd = false;
        }
        return *this;
    }
    BasicDiffCursor &operator+=(int offset)
    {
        *this += int64_t(offset);
        return *this;
    }

    bool moveChecked(int offset)
    {
        auto oldAddress = address;
        *this += offset;
        return address - oldAddress == uint64_t(offset);
    }

    BasicDiffCursor &operator+=(uint64_t offset)
    {
        if (uint64_t(offset) > (UINT64_MAX - address)) {
            address = UINT64_MAX;
            pastEnd = true;
        } else {
            address += offset;
            pastEnd = false;
        }
        return *this;
    }
    bool operator<(const BasicDiffCursor &r) const
    {
        return address < r.address || (pastEnd < r.pastEnd);
    }
};

/**
 * @brief Controls the cursor's visual blinking and screen location
 */
struct HexDiffCursor
{
    HexDiffCursor() : isVisible(false), onAsciiArea(false) {}

    bool isVisible;
    bool onAsciiArea;
    QTimer blinkTimer;
    QRectF screenPos;
    QRectF screenPosB;
    uint64_t address;
    QString cachedChar;
    QString cachedCharB;
    QColor cachedColor;
    QColor cachedColorB;

    void blink() { isVisible = !isVisible; }
    void setBlinkPeriod(int msec) { blinkTimer.setInterval(msec / 2); }
    void startBlinking() { blinkTimer.start(); }
    void stopBlinking() { blinkTimer.stop(); }
};

class HexDiffSelection
{
public:
    HexDiffSelection() : empty(true) { mStart = mEnd = 0; }

    inline void init(BasicDiffCursor addr)
    {
        empty = true;
        mInit = addr;
    }

    void set(uint64_t start, uint64_t end)
    {
        empty = false;
        mInit = BasicDiffCursor(start);
        mStart = start;
        mEnd = end;
    }

    void update(BasicDiffCursor addr)
    {
        empty = false;
        if (mInit < addr) {
            mStart = mInit.address;
            mEnd = addr.address;
            if (!addr.pastEnd) {
                mEnd -= 1;
            }
        } else if (addr < mInit) {
            mStart = addr.address;
            mEnd = mInit.address;
            if (!mInit.pastEnd) {
                mEnd -= 1;
            }
        } else {
            mStart = mEnd = mInit.address;
            empty = true;
        }
    }

    bool intersects(uint64_t start, uint64_t end) const
    {
        return !empty && mEnd >= start && mStart <= end;
    }

    bool contains(uint64_t pos) const { return !empty && mStart <= pos && pos <= mEnd; }

    uint64_t size() const
    {
        uint64_t size = 0;
        if (!isEmpty()) {
            size = mEnd - mStart + 1;
        }
        return size;
    }

    inline bool isEmpty() const { return empty; }
    inline uint64_t start() const { return mStart; }
    inline uint64_t end() const { return mEnd; }

private:
    BasicDiffCursor mInit;
    uint64_t mStart;
    uint64_t mEnd;
    bool empty;
};

class AddressRangeScrollBar;

/**
 * @brief Widget for rendering and editing of hex and ASCII memory data
 */
class HexDiff : public QScrollArea
{
    Q_OBJECT

public:
    explicit HexDiff(CutterDiff *cutterDiff, QWidget *parent = nullptr);
    ~HexDiff() override = default;

    void setMonospaceFont(const QFont &font);

    enum AddrWidth : ut8 { AddrWidth32 = 8, AddrWidth64 = 16 };
    enum ItemSize : ut8 {
        ItemSizeByte = 1,
        ItemSizeWord = 2,
        ItemSizeDword = 4,
        ItemSizeQword = 8
    };
    enum ItemFormat : ut8 {
        ItemFormatHex,
        ItemFormatOct,
        ItemFormatDec,
        ItemFormatSignedDec,
        ItemFormatFloat
    };

    enum class ColumnMode : ut8 { Fixed, PowerOf2 };
    enum class EditWordState : ut8 { Read, WriteNotStarted, WriteNotEdited, WriteEdited };
    enum class HexNavigationMode : ut8 { Words, WordChar, AnyChar };

    void setItemSize(int nbytes);
    void setItemFormat(ItemFormat format);
    void setItemEndianness(bool bigEndian);
    void setItemGroupSize(int size);
    /**
     * @brief Sets line size in bytes.
     * Changes column mode to fixed. Command can be rejected if current item format is bigger than
     * requested size.
     * @param bytes line size in bytes.
     */
    void setFixedLineSize(int bytes);
    void setColumnMode(ColumnMode mode);

    /**
     * @brief Select non empty inclusive range [start; end]
     * @param start
     * @param end
     */
    void selectRange(RVA start, RVA end);
    void clearSelection();

    void transpose(int transA = 0, int transB = 1);
    void shiftStartAddress(int shift);

    struct Selection
    {
        bool empty;
        RVA startAddress;
        RVA endAddress;
    };
    Selection getSelection();
public slots:
    void seek(uint64_t address, bool orig = true);
    void refresh();
    void updateColors();
signals:
    void selectionChanged(HexDiff::Selection selection);
    void positionChanged(RVA start);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onCursorBlinked();
    void onHexPairsModeEnabled(bool enable);
    void copy();
    void copyAddress();
    void onRangeDialogAccepted();

private:
    void updateItemLength();
    void updateCounts();
    void drawHeader(QPainter &painter, DiffFileContext &ctx);
    void drawCursor(QPainter &painter, bool shadow = false);
    void drawAddrArea(QPainter &painter, DiffFileContext &ctx);
    void drawItemArea(QPainter &painter, DiffFileContext &ctx);
    void drawAsciiArea(QPainter &painter, DiffFileContext &ctx);
    void fillSelectionBackground(QPainter &painter, DiffFileContext &ctx, bool ascii = false);
    void updateMetrics();
    void updateAreasPosition();
    void updateAreasHeight();
    enum class OverflowMove : ut8 { Clamp, Ignore };
    bool moveCursor(int offset, bool select = false,
                    OverflowMove overflowMove =
                            OverflowMove::Clamp); // The entire movecursor has to cursor shall be
                                                  // moved in same direction in both do with
                                                  // BasicDiffCursor and HexDiffSelection
    void moveCursorKeepEditOffset(int byteOffset, bool select, OverflowMove overflowMove);
    void setCursorAddr(BasicDiffCursor addr, bool select = false);
    void updateCursorMeta();
    void setCursorOnAscii(bool ascii);
    void setCursorOnArea(DiffArea area);
    QColor itemColor(uint8_t byte);
    QVariant readItem(int offset, DiffFileContext &ctx, QColor *color = nullptr);
    QString renderItem(int offset, DiffFileContext &ctx, QColor *color = nullptr);
    QChar renderAscii(int offset, DiffFileContext &ctx, QColor *color = nullptr);
    QString getFlagsAndComment(uint64_t address, DiffFileContext &ctx);
    /**
     * @brief Get the location on which operations such as Writing should apply.
     * @return Start of selection if multiple bytes are selected. Otherwise, the curren seek of the
     * widget.
     */
    RVA getLocationAddress();

    void fetchData();
    /**
     * @brief Convert mouse position to address.
     * @param point mouse position in widget
     * @param middle start next position from middle of symbol. Use middle=true for vertical cursor
     * position between symbols, middle=false for insert mode cursor and getting symbol under
     * cursor.
     * @return
     */
    BasicDiffCursor screenPosToAddrA(const QPoint &point, bool middle = false,
                                     int *wordOffset = nullptr) const;
    BasicDiffCursor asciiPosToAddrA(const QPoint &point, bool middle = false) const;
    BasicDiffCursor currentAreaPosToAddrA(const QPoint &point, bool middle = false) const;
    BasicDiffCursor mousePosToAddrA(const QPoint &point, bool middle = false) const;

    BasicDiffCursor screenPosToAddrB(const QPoint &point, bool middle = false,
                                     int *wordOffset = nullptr) const;
    BasicDiffCursor asciiPosToAddrB(const QPoint &point, bool middle = false) const;
    BasicDiffCursor currentAreaPosToAddrB(const QPoint &point, bool middle = false) const;
    BasicDiffCursor mousePosToAddrB(const QPoint &point, bool middle = false) const;
    DiffArea posToDiffArea(QPoint &point) const;
    /**
     * @brief Rectangle for single item in data area.
     * @param offset relative to first byte on screen
     * @return
     */
    QRectF itemRectangle(int offset, DiffFileContext &file);
    /**
     * @brief Rectangle for single item in ascii area.
     * @param offset relative to first byte on screen
     * @return
     */
    QRectF asciiRectangle(int offset, DiffFileContext &ctx);
    QVector<QPolygonF> rangePolygons(RVA start, RVA last, bool ascii, DiffFileContext &ctx);
    void updateWidth();

    inline qreal itemWidth() const { return itemCharLen * charWidth; }

    inline int itemGroupCharLen() const { return itemCharLen * itemGroupSize; }

    inline int columnExCharLen() const { return itemGroupCharLen() + columnSpacing; }

    inline int itemGroupByteLen() const { return itemByteLen * itemGroupSize; }

    inline qreal columnWidth() const { return itemGroupCharLen() * charWidth; }

    inline qreal columnExWidth() const { return columnExCharLen() * charWidth; }

    inline qreal columnSpacingWidth() const { return columnSpacing * charWidth; }

    inline int itemRowCharLen() const { return itemColumns * columnExCharLen() - columnSpacing; }

    inline int itemRowByteLen() const { return rowSizeBytes; }

    inline int bytesPerScreen() const { return itemRowByteLen() * visibleLines; }

    inline qreal itemRowWidth() const { return itemRowCharLen() * charWidth; }

    inline qreal asciiRowWidth() const { return itemRowByteLen() * charWidth; }

    inline qreal areaSpacingWidth() const { return areaSpacing * charWidth; }

    inline uint64_t lastVisibleAddr(DiffFile file) const
    {
        return ((file == DiffFile::A ? startAddress : getStartAddressB()) - 1) + bytesPerScreen();
    }

    const QRectF &currentArea();

    const QRectF &screenPosToArea(const QPoint &point) const;

    bool isFixedWidth() const;

    HexNavigationMode defaultNavigationMode();
    bool parseWord(const QString &word, uint8_t *buf, size_t bufferSize) const;

    void hideWarningRect();
    void showWarningRect(QRectF rect);

    void updateViewport();
    void scrollLines(int lines, bool clampToScrollBarRange = false);
    /**
     * @brief Sets the given address as the first visible address of the view
     * No action is taken if the address is already at the top
     * @param address Target RVA to display at the top
     */
    void setStartAddress(RVA address);

    uint64_t getAddressB(uint64_t addrA) const
    {
        if (relTranspose < 0) {
            return addrA - qAbs(relTranspose);
        }
        return addrA + relTranspose;
    }
    uint64_t getAddressA(uint64_t addrB) const
    {
        if (relTranspose < 0) {
            return addrB + qAbs(relTranspose);
        }
        return addrB - relTranspose;
    }

    uint64_t getStartAddressB() const { return getAddressB(startAddress); }

    /**
     * @brief Updates cursor visibility and metadata
     * The cursor is only visible if it is within the visible address range and selection is empty
     */
    void updateCursorStatus();

    bool cursorEnabled;
    DiffArea cursorArea;
    HexDiffCursor cursor;
    HexDiffCursor shadowCursor;

    HexDiffSelection selection;
    bool updatingSelection;

    int itemByteLen = 1;
    int itemGroupSize = 1; ///< Items per group (default: 1), 2 in case of hexpair mode
    int rowSizeBytes = 16; ///< Line size in bytes
    int itemColumns = 16; ///< Number of columns, single column consists of itemGroupSize items
    int itemCharLen = 2;
    int itemPrefixLen = 0;
    int relTranspose = 0; ///< relative transpose between the files
    ColumnMode columnMode;

    ItemFormat itemFormat;

    bool itemBigEndian;
    QString itemPrefix;

    int visibleLines;
    uint64_t startAddress;
    qreal charWidth;
    qreal lineHeight;
    int addrCharLen;
    QFont monospaceFont;

    bool showHeader;
    bool showAscii;
    bool showExHex;
    bool showExAddr;

    bool diffByteArrays(QByteArray &a, QByteArray &b);
    bool diffItemsAt(uint64_t addr);

    QColor borderColor;
    QColor backgroundColor;
    QColor defColor;
    QColor addrColor;
    QColor diffColor;
    QColor b0x00Color;
    QColor b0x7fColor;
    QColor b0xffColor;
    QColor printableColor;
    QColor warningColor;

    HexdumpRangeDialog rangeDialog;

    /* Spacings in characters */
    const int columnSpacing = 1;
    const int areaSpacing = 2;

    const QString hexPrefix = QStringLiteral("0x");

    QMenu *rowSizeMenu;
    QAction *actionRowSizePowerOf2;
    QList<QAction *> actionsItemSize;
    QList<QAction *> actionsItemFormat;
    QAction *actionItemBigEndian;
    QAction *actionHexPairs;
    QAction *actionCopy;
    QAction *actionCopyAddress;
    QAction *actionSelectRange;

    DiffFileContext ctxA;
    DiffFileContext ctxB;
    DiffFile getFileFromPos(QPoint &pos);
    HexDiffSelection ctxSelection(DiffFileContext &ctx);
    uint64_t ctxAddr(uint64_t addrA, DiffFileContext &ctx);

    HexNavigationMode navigationMode = HexNavigationMode::Words;

    bool warningRectVisible = false;
    QRectF warningRect;
    QTimer warningTimer;

    CutterDiff *cutterDiff;

    AddressRangeScrollBar *vScrollBar;
};

#endif // HEXWIDGET_H
