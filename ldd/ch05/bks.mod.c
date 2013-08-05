#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x49fb3e37, "module_layout" },
	{ 0x4a8efa93, "cdev_alloc" },
	{ 0xecb04dc9, "cdev_del" },
	{ 0xd231c89b, "cdev_init" },
	{ 0x9b388444, "get_zeroed_page" },
	{ 0x6980fe91, "param_get_int" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x973873ab, "_spin_lock" },
	{ 0xfc4f55f3, "down_interruptible" },
	{ 0xd3ad8b9a, "remove_proc_entry" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xff964b25, "param_set_int" },
	{ 0xb72397d5, "printk" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0x435b566d, "_spin_unlock" },
	{ 0xdf14c1c1, "cdev_add" },
	{ 0xf7158d50, "create_proc_entry" },
	{ 0x4302d0eb, "free_pages" },
	{ 0x3f1899f1, "up" },
	{ 0x701d0ebd, "snprintf" },
	{ 0xf2a644fb, "copy_from_user" },
	{ 0x29537c9e, "alloc_chrdev_region" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

