#include "exti.h"
#include "board_config.h"

/* EXTI line 8 <- PA8 (nut bam). Nut noi xuong GND va dung pull-up noi, nen su
 * kien nhan la CANH XUONG.
 *
 * VAI TRO: day la duong BO SUNG, khong thay the polling.
 * safety_monitor van doc nut qua button_read() o nhip 10 ms voi bo chong doi
 * bang phan mem, va do van la nguon su that duy nhat cho STOP/rearm. EXTI o
 * day chi de bat duoc mot lan nhan RAT NGAN lot giua hai lan poll, khi nao co
 * nhu cau phan ung nhanh hon 10 ms.
 *
 * VI SAO KHONG DE EXTI TU QUYET DINH: nut co dao (bounce) hang chuc lan trong
 * vai ms. EXTI se sinh tung ay ngat. ISR o day chi dat mot co — no KHONG dem,
 * khong phan biet nhan voi doi, va khong biet he thong dang bi inhibit hay
 * khong. Viec phan biet STOP voi rearm doi hoi trang thai, va trang thai do
 * thuoc safety_monitor.
 */

/* EXTI8 nam trong nhom EXTI9_5 — mot IRQ dung chung cho sau line. */
#define EXTI_LINE_BUTTON 8U
#define EXTI_MASK_BUTTON (1UL << EXTI_LINE_BUTTON)

static volatile bool edge_pending;
static bool initialized;

status_t exti_init(const exti_config_t *config)
{
    if (config == NULL || config->line != EXTI_LINE_BUTTON || config->priority < 5U ||
        config->priority > 15U) {
        return STATUS_ERROR;
    }
    initialized = false;
    edge_pending = false;

    /* Chan PA8 da duoc gpio_init() dat input pull-up. O day chi dinh tuyen
     * line 8 ve port A va bat bo phat hien canh — khong cau hinh lai chan,
     * de khong tao ra chu so huu thu hai cho no. */
    __HAL_RCC_AFIO_CLK_ENABLE();
    AFIO->EXTICR[EXTI_LINE_BUTTON / 4U] &=
        ~(uint32_t)(0xFUL << ((EXTI_LINE_BUTTON % 4U) * 4U));   /* 0 = GPIOA */

    EXTI->IMR &= ~EXTI_MASK_BUTTON;      /* tat truoc khi doi cau hinh */
    EXTI->RTSR &= ~EXTI_MASK_BUTTON;     /* khong bat canh len (nha nut) */
    EXTI->FTSR |= EXTI_MASK_BUTTON;      /* bat canh xuong (nhan nut) */
    EXTI->PR = EXTI_MASK_BUTTON;         /* xoa co con sot TRUOC khi bat NVIC */

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, config->priority, 0U);
    HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    EXTI->IMR |= EXTI_MASK_BUTTON;

    initialized = true;
    return STATUS_OK;
}

/* Doc VA tieu thu su kien: tra true dung mot lan cho moi canh xuong ghi nhan
 * duoc. Nguoi goi phai xu ly ngay, khong co hang doi phia sau.
 */
status_t exti_read_pending(bool *pending)
{
    if (pending == NULL) { return STATUS_ERROR; }
    *pending = false;
    if (!initialized) { return STATUS_NOT_READY; }

    /* Doc-roi-xoa. ISR chi ghi true nen mot canh den dung luc nay cung khong bi
     * mat: no se ghi lai true ngay sau. Truong hop xau nhat la mot canh duoc
     * bao hai lan, chu khong phai mat canh. */
    if (edge_pending) {
        edge_pending = false;
        *pending = true;
    }
    return STATUS_OK;
}

void exti_irq_capture(void)
{
    /* EXTI9_5 dung chung cho sau line. Chi xu ly line cua minh va chi xoa co
     * cua line do — xoa ca thanh ghi se nuot su kien cua module khac neu sau
     * nay co them chan trong nhom nay. */
    if ((EXTI->PR & EXTI_MASK_BUTTON) != 0U) {
        EXTI->PR = EXTI_MASK_BUTTON;   /* ghi 1 de xoa */
        edge_pending = true;
    }
}
