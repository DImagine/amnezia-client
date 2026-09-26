#pragma once

#include <QString>

// Validate both request identity and server/protocol before accepting an async result.
class ConnectionPreparationGuard
{
public:
    quint64 begin(const QString &context)
    {
        m_context = context;
        m_active = ++m_sequence;
        return m_active;
    }
    void cancel() { m_active = 0; }
    bool active() const { return m_active != 0; }
    bool consume(quint64 request, const QString &context)
    {
        if (!request || request != m_active)
            return false; // A late result must not consume a newer request.
        m_active = 0;
        return context == m_context;
    }

private:
    quint64 m_sequence = 0;
    quint64 m_active = 0;
    QString m_context;
};
