#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

#undef unix
struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = __stringify(KBUILD_MODNAME),
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x98af416e, "struct_module" },
	{ 0x188e584b, "del_timer" },
	{ 0xc192d491, "unregister_chrdev" },
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0x2071e84e, "register_chrdev" },
	{ 0xf20dabd8, "free_irq" },
	{ 0x99e3ad92, "kmem_cache_alloc" },
	{ 0x467e82db, "malloc_sizes" },
	{ 0xd49501d4, "__release_region" },
	{ 0x26e96637, "request_irq" },
	{ 0x1a1a4f09, "__request_region" },
	{ 0x865ebccd, "ioport_resource" },
	{ 0x1d26aa98, "sprintf" },
	{ 0x72270e35, "do_gettimeofday" },
	{ 0x97a80c66, "fasync_helper" },
	{ 0x75d5b568, "kill_fasync" },
	{ 0xd16ac615, "__get_user_1" },
	{ 0x1af40e18, "__copy_from_user_ll" },
	{ 0xdfb5f711, "__wake_up" },
	{ 0xd7474566, "__copy_to_user_ll" },
	{ 0x49e79940, "__cond_resched" },
	{ 0x9925ce9c, "__might_sleep" },
	{ 0x5eb3fe08, "interruptible_sleep_on_timeout" },
	{ 0xb2372b5b, "_spin_unlock_irqrestore" },
	{ 0x6804547e, "_spin_lock_irqsave" },
	{ 0x1b7d4074, "printk" },
	{ 0x89b301d4, "param_get_int" },
	{ 0x98bd6f46, "param_set_int" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";

