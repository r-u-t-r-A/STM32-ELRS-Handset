#include "hid_joystick.h"
#include "rc/rc_limits.h"

#ifdef USBD_USE_HID_COMPOSITE

#ifdef __cplusplus
extern "C" {
#endif
#include "usbd_core.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"
#include "usbd_desc.h"
#include "usbd_conf.h"
#ifdef __cplusplus
}
#endif

#define JOY_EPIN_ADDR      0x81U
#define JOY_REPORT_SIZE    (HID_JOY_AXIS_COUNT * 2 + 2)
#define JOY_EPIN_SIZE      JOY_REPORT_SIZE
#define JOY_FS_BINTERVAL   0x01U   /* ms */

/* The core's usbd_ep_conf.h puts EP 0x81 at PMA_EP0_IN_ADDR + 4, inside
 * EP0-IN's 64-byte buffer. Move it past EP0-IN: BTABLE (3 EPs x 8 bytes)
 * + EP0-OUT 64 + EP0-IN 64 = 152. */
#define JOY_PMA_ADDR       0x98U

#define JOY_HID_DESC_TYPE      0x21U
#define JOY_HID_REPORT_DESC    0x22U
#define JOY_HID_DESC_SIZE      9U
#define JOY_CONFIG_DESC_SIZE   34U

#define JOY_REQ_GET_REPORT     0x01U
#define JOY_REQ_GET_IDLE       0x02U
#define JOY_REQ_GET_PROTOCOL   0x03U
#define JOY_REQ_SET_IDLE       0x0AU
#define JOY_REQ_SET_PROTOCOL   0x0BU

__ALIGN_BEGIN static uint8_t joy_report_desc[] __ALIGN_END = {
    0x05, 0x01,             /* Usage Page (Generic Desktop) */
    0x09, 0x04,             /* Usage (Joystick) */
    0xA1, 0x01,             /* Collection (Application) */
    0x09, 0x01,             /*   Usage (Pointer) */
    0xA1, 0x00,             /*   Collection (Physical) */
    0x09, 0x30,             /*     Usage (X) */
    0x09, 0x31,             /*     Usage (Y) */
    0x09, 0x32,             /*     Usage (Z) */
    0x09, 0x33,             /*     Usage (Rx) */
    0x09, 0x34,             /*     Usage (Ry) */
    0x09, 0x35,             /*     Usage (Rz) */
    0x09, 0x36,             /*     Usage (Slider) */
    0x09, 0x37,             /*     Usage (Dial) */
    0x15, 0x00,             /*     Logical Minimum (0) */
    0x26, LOBYTE(HID_JOY_AXIS_MAX), HIBYTE(HID_JOY_AXIS_MAX), /* Logical Maximum */
    0x75, 0x10,             /*     Report Size (16) */
    0x95, HID_JOY_AXIS_COUNT, /*   Report Count */
    0x81, 0x02,             /*     Input (Data, Var, Abs) */
    0xC0,                   /*   End Collection */
    0x05, 0x09,             /*   Usage Page (Button) */
    0x19, 0x01,             /*   Usage Minimum (1) */
    0x29, HID_JOY_BUTTON_COUNT, /* Usage Maximum */
    0x15, 0x00,             /*   Logical Minimum (0) */
    0x25, 0x01,             /*   Logical Maximum (1) */
    0x75, 0x01,             /*   Report Size (1) */
    0x95, HID_JOY_BUTTON_COUNT, /* Report Count */
    0x81, 0x02,             /*   Input (Data, Var, Abs) */
    0x75, 16 - HID_JOY_BUTTON_COUNT, /* Report Size (pad to 16 bits) */
    0x95, 0x01,             /*   Report Count (1) */
    0x81, 0x03,             /*   Input (Const, Var, Abs) */
    0xC0                    /* End Collection */
};

__ALIGN_BEGIN static uint8_t joy_config_desc[JOY_CONFIG_DESC_SIZE] __ALIGN_END = {
    /* Configuration */
    0x09, USB_DESC_TYPE_CONFIGURATION,
    LOBYTE(JOY_CONFIG_DESC_SIZE), HIBYTE(JOY_CONFIG_DESC_SIZE),
    0x01,                   /* bNumInterfaces */
    0x01,                   /* bConfigurationValue */
    0x00,                   /* iConfiguration */
    0x80,                   /* bmAttributes: bus powered */
    USBD_MAX_POWER,         /* MaxPower */
    /* Interface */
    0x09, USB_DESC_TYPE_INTERFACE,
    0x00,                   /* bInterfaceNumber */
    0x00,                   /* bAlternateSetting */
    0x01,                   /* bNumEndpoints */
    0x03,                   /* bInterfaceClass: HID */
    0x00,                   /* bInterfaceSubClass: no boot */
    0x00,                   /* bInterfaceProtocol: none */
    0x00,                   /* iInterface */
    /* HID */
    JOY_HID_DESC_SIZE, JOY_HID_DESC_TYPE,
    0x11, 0x01,             /* bcdHID 1.11 */
    0x00,                   /* bCountryCode */
    0x01,                   /* bNumDescriptors */
    JOY_HID_REPORT_DESC,
    LOBYTE(sizeof(joy_report_desc)), HIBYTE(sizeof(joy_report_desc)),
    /* Endpoint IN */
    0x07, USB_DESC_TYPE_ENDPOINT,
    JOY_EPIN_ADDR,
    0x03,                   /* bmAttributes: interrupt */
    LOBYTE(JOY_EPIN_SIZE), HIBYTE(JOY_EPIN_SIZE),
    JOY_FS_BINTERVAL
};

__ALIGN_BEGIN static uint8_t joy_hid_desc[JOY_HID_DESC_SIZE] __ALIGN_END = {
    JOY_HID_DESC_SIZE, JOY_HID_DESC_TYPE,
    0x11, 0x01,
    0x00,
    0x01,
    JOY_HID_REPORT_DESC,
    LOBYTE(sizeof(joy_report_desc)), HIBYTE(sizeof(joy_report_desc))
};

__ALIGN_BEGIN static uint8_t joy_qualifier_desc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC, USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00
};

static struct {
    volatile uint8_t busy;
    uint8_t protocol;
    uint8_t idle;
    uint8_t alt_setting;
} joy_state;

__ALIGN_BEGIN static uint8_t joy_report[JOY_REPORT_SIZE] __ALIGN_END;

static USBD_HandleTypeDef hUsbJoystick;

static uint8_t joy_init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    (void)cfgidx;
    pdev->pClassDataCmsit[pdev->classId] = (void *)&joy_state;
    pdev->pClassData = pdev->pClassDataCmsit[pdev->classId];

    pdev->ep_in[JOY_EPIN_ADDR & 0xFU].bInterval = JOY_FS_BINTERVAL;
    (void)USBD_LL_OpenEP(pdev, JOY_EPIN_ADDR, USBD_EP_TYPE_INTR, JOY_EPIN_SIZE);
    pdev->ep_in[JOY_EPIN_ADDR & 0xFU].is_used = 1U;
    joy_state.busy = 0;
    return (uint8_t)USBD_OK;
}

static uint8_t joy_deinit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    (void)cfgidx;
    (void)USBD_LL_CloseEP(pdev, JOY_EPIN_ADDR);
    pdev->ep_in[JOY_EPIN_ADDR & 0xFU].is_used = 0U;
    pdev->ep_in[JOY_EPIN_ADDR & 0xFU].bInterval = 0U;
    pdev->pClassDataCmsit[pdev->classId] = NULL;
    pdev->pClassData = NULL;
    joy_state.busy = 0;
    return (uint8_t)USBD_OK;
}

static uint8_t joy_setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    static uint16_t status_info = 0U;
    USBD_StatusTypeDef ret = USBD_OK;

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_CLASS:
        switch (req->bRequest) {
        case JOY_REQ_SET_PROTOCOL:
            joy_state.protocol = (uint8_t)req->wValue;
            break;
        case JOY_REQ_GET_PROTOCOL:
            (void)USBD_CtlSendData(pdev, &joy_state.protocol, 1U);
            break;
        case JOY_REQ_SET_IDLE:
            joy_state.idle = (uint8_t)(req->wValue >> 8);
            break;
        case JOY_REQ_GET_IDLE:
            (void)USBD_CtlSendData(pdev, &joy_state.idle, 1U);
            break;
        case JOY_REQ_GET_REPORT:
            (void)USBD_CtlSendData(pdev, joy_report, MIN((uint16_t)JOY_REPORT_SIZE, req->wLength));
            break;
        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
        }
        break;

    case USB_REQ_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_STATUS:
            if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
            } else {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;
        case USB_REQ_GET_DESCRIPTOR:
            if ((req->wValue >> 8) == JOY_HID_REPORT_DESC) {
                (void)USBD_CtlSendData(pdev, joy_report_desc,
                                       MIN((uint16_t)sizeof(joy_report_desc), req->wLength));
            } else if ((req->wValue >> 8) == JOY_HID_DESC_TYPE) {
                (void)USBD_CtlSendData(pdev, joy_hid_desc,
                                       MIN((uint16_t)JOY_HID_DESC_SIZE, req->wLength));
            } else {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;
        case USB_REQ_GET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                (void)USBD_CtlSendData(pdev, &joy_state.alt_setting, 1U);
            } else {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;
        case USB_REQ_SET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                joy_state.alt_setting = (uint8_t)req->wValue;
            } else {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;
        case USB_REQ_CLEAR_FEATURE:
            break;
        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
        }
        break;

    default:
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
        break;
    }
    return (uint8_t)ret;
}

static uint8_t joy_data_in(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    (void)pdev;
    if (epnum == (JOY_EPIN_ADDR & 0x7FU)) {
        joy_state.busy = 0;
    }
    return (uint8_t)USBD_OK;
}

static uint8_t *joy_get_config_desc(uint16_t *length) {
    *length = (uint16_t)sizeof(joy_config_desc);
    return joy_config_desc;
}

static uint8_t *joy_get_qualifier_desc(uint16_t *length) {
    *length = (uint16_t)sizeof(joy_qualifier_desc);
    return joy_qualifier_desc;
}

static USBD_ClassTypeDef USBD_JOYSTICK_CLASS = {
    joy_init,
    joy_deinit,
    joy_setup,
    NULL,                   /* EP0_TxSent */
    NULL,                   /* EP0_RxReady */
    joy_data_in,
    NULL,                   /* DataOut */
    NULL,                   /* SOF */
    NULL,                   /* IsoINIncomplete */
    NULL,                   /* IsoOUTIncomplete */
    joy_get_config_desc,    /* HS */
    joy_get_config_desc,    /* FS */
    joy_get_config_desc,    /* other speed */
    joy_get_qualifier_desc,
};

void hid_joystick_begin(void) {
    if (USBD_Init(&hUsbJoystick, &USBD_Desc, 0) != USBD_OK) {
        return;
    }
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef *)hUsbJoystick.pData,
                        JOY_EPIN_ADDR, PCD_SNG_BUF, JOY_PMA_ADDR);
    if (USBD_RegisterClass(&hUsbJoystick, &USBD_JOYSTICK_CLASS) != USBD_OK) {
        return;
    }
    USBD_Start(&hUsbJoystick);
}

static uint16_t joy_axis(int ch) {
    int32_t v = ((int32_t)ch - RC_CHANNEL_MIN) * HID_JOY_AXIS_MAX
                / (RC_CHANNEL_MAX - RC_CHANNEL_MIN);
    if (v < 0) {
        v = 0;
    }
    if (v > HID_JOY_AXIS_MAX) {
        v = HID_JOY_AXIS_MAX;
    }
    return (uint16_t)v;
}

void hid_joystick_send(const int *channels) {
    if (hUsbJoystick.dev_state != USBD_STATE_CONFIGURED || joy_state.busy) {
        return;
    }

    for (uint8_t i = 0; i < HID_JOY_AXIS_COUNT; i++) {
        uint16_t v = joy_axis(channels[i]);
        joy_report[i * 2] = LOBYTE(v);
        joy_report[i * 2 + 1] = HIBYTE(v);
    }

    uint16_t buttons = 0;
    for (uint8_t i = 0; i < HID_JOY_BUTTON_COUNT; i++) {
        if (channels[HID_JOY_BUTTON_FIRST + i] > RC_CHANNEL_MID) {
            buttons |= (uint16_t)(1U << i);
        }
    }
    joy_report[HID_JOY_AXIS_COUNT * 2] = LOBYTE(buttons);
    joy_report[HID_JOY_AXIS_COUNT * 2 + 1] = HIBYTE(buttons);

    joy_state.busy = 1;
    (void)USBD_LL_Transmit(&hUsbJoystick, JOY_EPIN_ADDR, joy_report, JOY_REPORT_SIZE);
}

uint8_t hid_joystick_connected(void) {
    return hUsbJoystick.dev_state == USBD_STATE_CONFIGURED;
}

#else /* !USBD_USE_HID_COMPOSITE: built without USB, SIM is a no-op */

void hid_joystick_begin(void) {}
void hid_joystick_send(const int *channels) { (void)channels; }
uint8_t hid_joystick_connected(void) { return 0; }

#endif
