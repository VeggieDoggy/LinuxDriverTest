#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x3e549f1d, "module_layout" },
	{ 0x4202ea9c, "class_destroy" },
	{ 0xb013481f, "platform_driver_unregister" },
	{ 0xd7362d1, "__platform_driver_register" },
	{ 0x5bbe49f4, "__init_waitqueue_head" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xb47e05b2, "__class_create" },
	{ 0x28908313, "__register_chrdev" },
	{ 0x49970de8, "finish_wait" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0x647af474, "prepare_to_wait_event" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x189c5980, "arm_copy_to_user" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x3dcf1ffa, "__wake_up" },
	{ 0xc4f0da12, "ktime_get_with_offset" },
	{ 0xb4fe4c70, "device_create" },
	{ 0x5154e274, "gpiod_to_irq" },
	{ 0xd8a801f2, "gpiod_get" },
	{ 0xd840a947, "gpiod_put" },
	{ 0x46168736, "device_destroy" },
	{ 0xc5850110, "printk" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

MODULE_INFO(depends, "");

