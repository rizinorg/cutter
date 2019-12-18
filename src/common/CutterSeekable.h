#pragma once

#include "core/Cutter.h"

class MainWindow;

class CutterSeekable : public QObject
{
    Q_OBJECT

public:
    explicit CutterSeekable(QObject *parent = nullptr);
    ~CutterSeekable();

    /**
     * @brief seek changes current offset.
     * If the seekable is synchronized with Core, then
     * the Core offset will be modified and then the CutterCore::seekChanged
     * signal will be emitted.
     * In any case, CutterSeekable::seekableSeekChanged is emitted.
     * @param addr the location to seek at.
     */
    void seek(RVA addr) { updateSeek(addr, false); }

    /**
     * @brief setSynchronization sets
     * Core seek synchronization.
     */
    void setSynchronization(bool sync);

    /**
     * @brief getOffset returns the seekable offset.
     * If the seekable is synchronized with Core, this function
     * is similar to Core()->getOffset.
     * If it's not synchronized, it will return the seekable current seek.
     * @return the seekable current offset.
     */
    RVA getOffset();

    /**
     * @brief isSynchronized tells whether the seekable
     * is synchronized with Core or not.
     * @return boolean
     */
    bool isSynchronized();

    /**
     * @brief seekToReference will seek to the function or the object which is referenced in a given offset
     * @param offset - an address that contains a reference to jump to
     */
    void seekToReference(RVA offset);

public slots:
    /**
     * @brief seekPrev seeks to last location.
     */
    void seekPrev();

    /**
     * @brief toggleSyncWithCore toggles
     * Core seek synchronization.
     */
    void toggleSynchronization();

private slots:
    /**
     * @brief onCoreSeekChanged
     */
    void onCoreSeekChanged(RVA addr);

private:
    /**
     * @brief widgetOffset widget seek location.
     */
    RVA widgetOffset = RVA_INVALID;

    /**
     * @brief previousOffset last seek location.
     * @todo maybe use an actual history?
     */
    RVA previousOffset = RVA_INVALID;

    /**
     * @brief synchronized tells with the seekable's offset is
     * synchronized with core or not.
     */
    bool synchronized = true;

    /**
     * @brief internal method for changing the seek
     * @param localOnly whether the seek should be updated globally if synchronized
     */
    void updateSeek(RVA addr, bool localOnly);

signals:
    void seekableSeekChanged(RVA addr);
    void syncChanged();
};
