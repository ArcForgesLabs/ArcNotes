#ifndef SUBSCRIPTIONSTATUS_H
#define SUBSCRIPTIONSTATUS_H

#include <QMetaType>
#include <cstdint>

class SubscriptionStatus {
    Q_GADGET

public:
    enum Value : uint8_t {
        NoSubscription,
        Active,
        ActivationLimitReached,
        Expired,
        Invalid,
        EnteredGracePeriod,
        GracePeriodOver,
        NoInternetConnection,
        UnknownError
    };
    Q_ENUM(Value)

    static int qRegisterMetaType() { return ::qRegisterMetaType<SubscriptionStatus::Value>("SubscriptionStatus"); }

    static void registerEnum(const char* uri, int major, int minor) {
        Q_UNUSED(uri);
        Q_UNUSED(major);
        Q_UNUSED(minor);
        SubscriptionStatus::qRegisterMetaType();
    }
};

Q_DECLARE_METATYPE(SubscriptionStatus::Value)

#endif  // SUBSCRIPTIONSTATUS_H
