#include "PjHandle.h"

#include <rz_util/rz_pj.h>

PjHandle::PjHandle() : m_pj(pj_new()) {}

PjHandle::~PjHandle() = default;

PjHandle::PjHandle(PjHandle &&) noexcept = default;

PjHandle &PjHandle::operator=(PjHandle &&other) noexcept
{
    // unique_ptr self move isn't safe, so guarding.
    if (this != &other) {
        m_pj = std::move(other.m_pj);
    }
    return *this;
}

char *PjHandle::drain()
{
    PJ *pj = m_pj.release();
    if (!pj) {
        return nullptr;
    }
    return pj_drain(pj);
}
