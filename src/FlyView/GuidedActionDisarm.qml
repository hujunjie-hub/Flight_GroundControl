import QGroundControl
import QGroundControl.FlyView

/// Fly view tool strip entry which disarms the vehicle through COMPONENT_ARM_DISARM.
GuidedToolStripAction {
    text:       _guidedController.disarmTitle
    iconSource: "/res/LockClosed.svg"
    visible:    _guidedController.showDisarm
    enabled:    _guidedController.showDisarm
    actionID:   _guidedController.actionDisarm
}
