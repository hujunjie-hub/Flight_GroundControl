import QGroundControl
import QGroundControl.FlyView

/// Fly view tool strip entry which arms the vehicle through COMPONENT_ARM_DISARM.
/// QGC 5.1.5 ships no arm/disarm entry in FlyViewToolStripActionList, so a vehicle whose
/// firmware requires an explicit arm before takeoff had no way to be armed from the Fly view.
GuidedToolStripAction {
    text:       _guidedController.armTitle
    iconSource: "/res/LockOpen.svg"
    visible:    _guidedController.showArm
    enabled:    _guidedController.showArm
    actionID:   _guidedController.actionArm
}
