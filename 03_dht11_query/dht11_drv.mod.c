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
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x4202ea9c, "class_destroy" },
	{ 0xb013481f, "platform_driver_unregister" },
	{ 0xd7362d1, "__platform_driver_register" },
	{ 0xb47e05b2, "__class_create" },
	{ 0x28908313, "__register_chrdev" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x189c5980, "arm_copy_to_user" },
	{ 0x1a029d1b, "gpiod_direction_output" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xb4fe4c70, "device_create" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0x5154e274, "gpiod_to_irq" },
	{ 0xd8a801f2, "gpiod_get" },
	{ 0x3dcf1ffa, "__wake_up" },
	{ 0xfd1c5168, "gpiod_get_value" },
	{ 0xd840a947, "gpiod_put" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x46168736, "device_destroy" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xc5850110, "printk" },
};

MODULE_INFO(depends, "");

