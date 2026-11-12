#ifndef PJHANDLE_H
#define PJHANDLE_H

#include "core/CutterCommon.h"

#include <memory>

/**
 * @brief RAII handle for a rizin PJ (JSON builder).
 *
 * Owns a pj_new() alloc, the destructor calls pj_free() on early return
 * paths before the PJ is drained. drain() releases ownership (pj_drain frees
 * the PJ and returns its buffer), so the destructor is a no-op after drain.
 *
 * move used, as copying would alias the PJ and do double free.
 *
 * @code
 * {
 *     PjHandle pj;
 *     if (!pj) {
 *         return;
 *     }
 *     pj_o(pj.get());
 *     pj_ks(pj.get(), "name", "value");
 *     pj_end(pj.get());
 *     char *json = pj.drain();
 *     // caller frees json
 * }
 * @endcode
 */
class CUTTER_EXPORT PjHandle
{
public:
    PjHandle();
    ~PjHandle();

    PjHandle(const PjHandle &) = delete;
    PjHandle &operator=(const PjHandle &) = delete;
    PjHandle(PjHandle &&other) noexcept;
    PjHandle &operator=(PjHandle &&other) noexcept;

    explicit operator bool() const { return m_pj != nullptr; }
    PJ *get() const { return m_pj.get(); }

    /**
     * @brief Drain the PJ into its JSON buffer.
     * @return buffer of the drained PJ, or nullptr if already drained. The
     * caller takes ownership and must free() the returned buffer.
     */
    char *drain();

private:
    struct Deleter
    {
        void operator()(PJ *pj) const { pj_free(pj); }
    };
    std::unique_ptr<PJ, Deleter> m_pj;
};

#endif // PJHANDLE_H
