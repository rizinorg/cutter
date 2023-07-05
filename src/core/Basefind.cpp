#include "Basefind.h"

bool Basefind::threadCallback(const RzBaseFindThreadInfo *info, void *user)
{
    auto th = reinterpret_cast<Basefind *>(user);
    return th->updateProgress(info);
}

Basefind::Basefind(CutterCore *core)
    : core(core),
      scores(nullptr),
      continue_run(true)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      mutex(QMutex::Recursive)
#endif
{
    memset(&options, 0, sizeof(RzBaseFindOpt));
}

Basefind::~Basefind()
{
    cancel();
    wait();
    rz_list_free(scores);
}

bool Basefind::setOptions(const RzBaseFindOpt *opts)
{
    mutex.lock();
    options.max_threads = opts->max_threads;
    options.pointer_size = opts->pointer_size;
    options.start_address = opts->start_address;
    options.end_address = opts->end_address;
    options.alignment = opts->alignment;
    options.min_score = opts->min_score;
    options.min_string_len = opts->min_string_len;
    mutex.unlock();

    if (options.start_address >= options.end_address) {
        qWarning() << tr("Start address is >= end address");
        return false;
    } else if (options.alignment < RZ_BASEFIND_BASE_ALIGNMENT) {
        qWarning() << tr("Alignment must be at least ")
                   << QString::asprintf("0x%x", RZ_BASEFIND_BASE_ALIGNMENT);
        return false;
    } else if (options.min_score < 1) {
        qWarning() << tr("Min score must be at least 1");
        return false;
    } else if (options.min_string_len < 1) {
        qWarning() << tr("Min string length must be at least 1");
        return false;
    }
    return true;
}

void Basefind::run()
{
    qRegisterMetaType<BasefindCoreStatusDescription>();

    mutex.lock();
    rz_list_free(scores);
    scores = nullptr;
    continue_run = true;
    mutex.unlock();

    core->coreMutex.lock();
    options.callback = threadCallback;
    options.user = this;
    scores = rz_basefind(core->core_, &options);
    core->coreMutex.unlock();

    emit complete();
}

void Basefind::cancel()
{
    mutex.lock();
    continue_run = false;
    mutex.unlock();
}

QList<BasefindResultDescription> Basefind::results()
{
    QList<BasefindResultDescription> pairs;
    RzListIter *it;
    RzBaseFindScore *pair;
    CutterRzListForeach (scores, it, RzBaseFindScore, pair) {
        BasefindResultDescription desc;
        desc.candidate = pair->candidate;
        desc.score = pair->score;
        pairs.push_back(desc);
    }
    return pairs;
}

bool Basefind::updateProgress(const RzBaseFindThreadInfo *info)
{
    mutex.lock();

    BasefindCoreStatusDescription status;
    status.index = info->thread_idx;
    status.percentage = info->percentage;

    emit progress(status);
    bool ret = continue_run;
    mutex.unlock();
    return ret;
}
