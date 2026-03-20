pragma Singleton

import QtQml

QtObject {
    // Mirror of SubscriptionStatus::Value in C++.
    enum Value {
        NoSubscription,
        Active,
        ActivationLimitReached,
        Expired,
        Invalid,
        EnteredGracePeriod,
        GracePeriodOver,
        NoInternetConnection,
        UnknownError
    }
}
