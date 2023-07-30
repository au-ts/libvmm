const libsel4cp = @cImport({
    @cInclude("libsel4cp.h");
});

pub const __builtin_bswap16 = @import("std").zig.c_builtins.__builtin_bswap16;
pub const __builtin_bswap32 = @import("std").zig.c_builtins.__builtin_bswap32;
pub const __builtin_bswap64 = @import("std").zig.c_builtins.__builtin_bswap64;
pub const __builtin_signbit = @import("std").zig.c_builtins.__builtin_signbit;
pub const __builtin_signbitf = @import("std").zig.c_builtins.__builtin_signbitf;
pub const __builtin_popcount = @import("std").zig.c_builtins.__builtin_popcount;
pub const __builtin_ctz = @import("std").zig.c_builtins.__builtin_ctz;
pub const __builtin_clz = @import("std").zig.c_builtins.__builtin_clz;
pub const __builtin_sqrt = @import("std").zig.c_builtins.__builtin_sqrt;
pub const __builtin_sqrtf = @import("std").zig.c_builtins.__builtin_sqrtf;
pub const __builtin_sin = @import("std").zig.c_builtins.__builtin_sin;
pub const __builtin_sinf = @import("std").zig.c_builtins.__builtin_sinf;
pub const __builtin_cos = @import("std").zig.c_builtins.__builtin_cos;
pub const __builtin_cosf = @import("std").zig.c_builtins.__builtin_cosf;
pub const __builtin_exp = @import("std").zig.c_builtins.__builtin_exp;
pub const __builtin_expf = @import("std").zig.c_builtins.__builtin_expf;
pub const __builtin_exp2 = @import("std").zig.c_builtins.__builtin_exp2;
pub const __builtin_exp2f = @import("std").zig.c_builtins.__builtin_exp2f;
pub const __builtin_log = @import("std").zig.c_builtins.__builtin_log;
pub const __builtin_logf = @import("std").zig.c_builtins.__builtin_logf;
pub const __builtin_log2 = @import("std").zig.c_builtins.__builtin_log2;
pub const __builtin_log2f = @import("std").zig.c_builtins.__builtin_log2f;
pub const __builtin_log10 = @import("std").zig.c_builtins.__builtin_log10;
pub const __builtin_log10f = @import("std").zig.c_builtins.__builtin_log10f;
pub const __builtin_abs = @import("std").zig.c_builtins.__builtin_abs;
pub const __builtin_fabs = @import("std").zig.c_builtins.__builtin_fabs;
pub const __builtin_fabsf = @import("std").zig.c_builtins.__builtin_fabsf;
pub const __builtin_floor = @import("std").zig.c_builtins.__builtin_floor;
pub const __builtin_floorf = @import("std").zig.c_builtins.__builtin_floorf;
pub const __builtin_ceil = @import("std").zig.c_builtins.__builtin_ceil;
pub const __builtin_ceilf = @import("std").zig.c_builtins.__builtin_ceilf;
pub const __builtin_trunc = @import("std").zig.c_builtins.__builtin_trunc;
pub const __builtin_truncf = @import("std").zig.c_builtins.__builtin_truncf;
pub const __builtin_round = @import("std").zig.c_builtins.__builtin_round;
pub const __builtin_roundf = @import("std").zig.c_builtins.__builtin_roundf;
pub const __builtin_strlen = @import("std").zig.c_builtins.__builtin_strlen;
pub const __builtin_strcmp = @import("std").zig.c_builtins.__builtin_strcmp;
pub const __builtin_object_size = @import("std").zig.c_builtins.__builtin_object_size;
pub const __builtin___memset_chk = @import("std").zig.c_builtins.__builtin___memset_chk;
pub const __builtin_memset = @import("std").zig.c_builtins.__builtin_memset;
pub const __builtin___memcpy_chk = @import("std").zig.c_builtins.__builtin___memcpy_chk;
pub const __builtin_memcpy = @import("std").zig.c_builtins.__builtin_memcpy;
pub const __builtin_expect = @import("std").zig.c_builtins.__builtin_expect;
pub const __builtin_nanf = @import("std").zig.c_builtins.__builtin_nanf;
pub const __builtin_huge_valf = @import("std").zig.c_builtins.__builtin_huge_valf;
pub const __builtin_inff = @import("std").zig.c_builtins.__builtin_inff;
pub const __builtin_isnan = @import("std").zig.c_builtins.__builtin_isnan;
pub const __builtin_isinf = @import("std").zig.c_builtins.__builtin_isinf;
pub const __builtin_isinf_sign = @import("std").zig.c_builtins.__builtin_isinf_sign;
pub const __has_builtin = @import("std").zig.c_builtins.__has_builtin;
pub const __builtin_assume = @import("std").zig.c_builtins.__builtin_assume;
pub const __builtin_unreachable = @import("std").zig.c_builtins.__builtin_unreachable;
pub const __builtin_constant_p = @import("std").zig.c_builtins.__builtin_constant_p;
pub const __builtin_mul_overflow = @import("std").zig.c_builtins.__builtin_mul_overflow;
pub const int_least64_t = i64;
pub const uint_least64_t = u64;
pub const int_fast64_t = i64;
pub const uint_fast64_t = u64;
pub const int_least32_t = i32;
pub const uint_least32_t = u32;
pub const int_fast32_t = i32;
pub const uint_fast32_t = u32;
pub const int_least16_t = i16;
pub const uint_least16_t = u16;
pub const int_fast16_t = i16;
pub const uint_fast16_t = u16;
pub const int_least8_t = i8;
pub const uint_least8_t = u8;
pub const int_fast8_t = i8;
pub const uint_fast8_t = u8;
pub const intmax_t = c_long;
pub const uintmax_t = c_ulong;
pub const seL4_Int8 = i8;
pub const seL4_Uint8 = u8; // ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
pub const seL4_Int16 = c_short;
pub const seL4_Uint16 = c_ushort; // ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
pub const seL4_Int32 = c_int;
pub const seL4_Uint32 = c_uint; // ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
pub const seL4_Int64 = c_long;
pub const seL4_Uint64 = c_ulong;
pub const seL4_Bool = seL4_Int8;
pub const seL4_Word = seL4_Uint64;
pub const seL4_CPtr = seL4_Word;
pub const seL4_UnknownSyscall_X0: c_int = 0;
pub const seL4_UnknownSyscall_X1: c_int = 1;
pub const seL4_UnknownSyscall_X2: c_int = 2;
pub const seL4_UnknownSyscall_X3: c_int = 3;
pub const seL4_UnknownSyscall_X4: c_int = 4;
pub const seL4_UnknownSyscall_X5: c_int = 5;
pub const seL4_UnknownSyscall_X6: c_int = 6;
pub const seL4_UnknownSyscall_X7: c_int = 7;
pub const seL4_UnknownSyscall_FaultIP: c_int = 8;
pub const seL4_UnknownSyscall_SP: c_int = 9;
pub const seL4_UnknownSyscall_LR: c_int = 10;
pub const seL4_UnknownSyscall_SPSR: c_int = 11;
pub const seL4_UnknownSyscall_Syscall: c_int = 12;
pub const seL4_UnknownSyscall_Length: c_int = 13;
pub const _enum_pad_seL4_UnknownSyscall_Msg: c_ulong = 9223372036854775807;
pub const seL4_UnknownSyscall_Msg = c_ulong;
pub const seL4_UserException_FaultIP: c_int = 0;
pub const seL4_UserException_SP: c_int = 1;
pub const seL4_UserException_SPSR: c_int = 2;
pub const seL4_UserException_Number: c_int = 3;
pub const seL4_UserException_Code: c_int = 4;
pub const seL4_UserException_Length: c_int = 5;
pub const _enum_pad_seL4_UserException_Msg: c_ulong = 9223372036854775807;
pub const seL4_UserException_Msg = c_ulong;
pub const seL4_VMFault_IP: c_int = 0;
pub const seL4_VMFault_Addr: c_int = 1;
pub const seL4_VMFault_PrefetchFault: c_int = 2;
pub const seL4_VMFault_FSR: c_int = 3;
pub const seL4_VMFault_Length: c_int = 4;
pub const _enum_pad_seL4_VMFault_Msg: c_ulong = 9223372036854775807;
pub const seL4_VMFault_Msg = c_ulong;
pub const seL4_VGICMaintenance_IDX: c_int = 0;
pub const seL4_VGICMaintenance_Length: c_int = 1;
pub const _enum_pad_seL4_VGICMaintenance_Msg: c_ulong = 9223372036854775807;
pub const seL4_VGICMaintenance_Msg = c_ulong;
pub const seL4_VPPIEvent_IRQ: c_int = 0;
pub const _enum_pad_seL4_VPPIEvent_Msg: c_ulong = 9223372036854775807;
pub const seL4_VPPIEvent_Msg = c_ulong;
pub const seL4_VCPUFault_HSR: c_int = 0;
pub const seL4_VCPUFault_Length: c_int = 1;
pub const _enum_pad_seL4_VCPUFault_Msg: c_ulong = 9223372036854775807;
pub const seL4_VCPUFault_Msg = c_ulong;
pub const seL4_VCPUReg_SCTLR: c_int = 0;
pub const seL4_VCPUReg_TTBR0: c_int = 1;
pub const seL4_VCPUReg_TTBR1: c_int = 2;
pub const seL4_VCPUReg_TCR: c_int = 3;
pub const seL4_VCPUReg_MAIR: c_int = 4;
pub const seL4_VCPUReg_AMAIR: c_int = 5;
pub const seL4_VCPUReg_CIDR: c_int = 6;
pub const seL4_VCPUReg_ACTLR: c_int = 7;
pub const seL4_VCPUReg_CPACR: c_int = 8;
pub const seL4_VCPUReg_AFSR0: c_int = 9;
pub const seL4_VCPUReg_AFSR1: c_int = 10;
pub const seL4_VCPUReg_ESR: c_int = 11;
pub const seL4_VCPUReg_FAR: c_int = 12;
pub const seL4_VCPUReg_ISR: c_int = 13;
pub const seL4_VCPUReg_VBAR: c_int = 14;
pub const seL4_VCPUReg_TPIDR_EL1: c_int = 15;
pub const seL4_VCPUReg_SP_EL1: c_int = 16;
pub const seL4_VCPUReg_ELR_EL1: c_int = 17;
pub const seL4_VCPUReg_SPSR_EL1: c_int = 18;
pub const seL4_VCPUReg_CNTV_CTL: c_int = 19;
pub const seL4_VCPUReg_CNTV_CVAL: c_int = 20;
pub const seL4_VCPUReg_CNTVOFF: c_int = 21;
pub const seL4_VCPUReg_CNTKCTL_EL1: c_int = 22;
pub const seL4_VCPUReg_Num: c_int = 23;
pub const seL4_VCPUReg = c_uint;
pub const seL4_TimeoutReply_FaultIP: c_int = 0;
pub const seL4_TimeoutReply_SP: c_int = 1;
pub const seL4_TimeoutReply_SPSR_EL1: c_int = 2;
pub const seL4_TimeoutReply_X0: c_int = 3;
pub const seL4_TimeoutReply_X1: c_int = 4;
pub const seL4_TimeoutReply_X2: c_int = 5;
pub const seL4_TimeoutReply_X3: c_int = 6;
pub const seL4_TimeoutReply_X4: c_int = 7;
pub const seL4_TimeoutReply_X5: c_int = 8;
pub const seL4_TimeoutReply_X6: c_int = 9;
pub const seL4_TimeoutReply_X7: c_int = 10;
pub const seL4_TimeoutReply_X8: c_int = 11;
pub const seL4_TimeoutReply_X16: c_int = 12;
pub const seL4_TimeoutReply_X17: c_int = 13;
pub const seL4_TimeoutReply_X18: c_int = 14;
pub const seL4_TimeoutReply_X29: c_int = 15;
pub const seL4_TimeoutReply_X30: c_int = 16;
pub const seL4_TimeoutReply_X9: c_int = 17;
pub const seL4_TimeoutReply_X10: c_int = 18;
pub const seL4_TimeoutReply_X11: c_int = 19;
pub const seL4_TimeoutReply_X12: c_int = 20;
pub const seL4_TimeoutReply_X13: c_int = 21;
pub const seL4_TimeoutReply_X14: c_int = 22;
pub const seL4_TimeoutReply_X15: c_int = 23;
pub const seL4_TimeoutReply_X19: c_int = 24;
pub const seL4_TimeoutReply_X20: c_int = 25;
pub const seL4_TimeoutReply_X21: c_int = 26;
pub const seL4_TimeoutReply_X22: c_int = 27;
pub const seL4_TimeoutReply_X23: c_int = 28;
pub const seL4_TimeoutReply_X24: c_int = 29;
pub const seL4_TimeoutReply_X25: c_int = 30;
pub const seL4_TimeoutReply_X26: c_int = 31;
pub const seL4_TimeoutReply_X27: c_int = 32;
pub const seL4_TimeoutReply_X28: c_int = 33;
pub const seL4_TimeoutReply_Length: c_int = 34;
pub const _enum_pad_seL4_TimeoutReply_Msg: c_ulong = 9223372036854775807;
pub const seL4_TimeoutReply_Msg = c_ulong;
pub const seL4_Timeout_Data: c_int = 0;
pub const seL4_Timeout_Consumed: c_int = 1;
pub const seL4_Timeout_Length: c_int = 2;
pub const _enum_pad_seL4_Timeout_Msg: c_ulong = 9223372036854775807;
pub const seL4_TimeoutMsg = c_ulong; // ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
pub const seL4_ARM_PageUpperDirectory = seL4_CPtr;
pub const seL4_ARM_PageGlobalDirectory = seL4_CPtr;
pub const seL4_ARM_VSpace = seL4_CPtr;
pub const struct_seL4_UserContext_ = extern struct {
    pc: seL4_Word,
    sp: seL4_Word,
    spsr: seL4_Word,
    x0: seL4_Word,
    x1: seL4_Word,
    x2: seL4_Word,
    x3: seL4_Word,
    x4: seL4_Word,
    x5: seL4_Word,
    x6: seL4_Word,
    x7: seL4_Word,
    x8: seL4_Word,
    x16: seL4_Word,
    x17: seL4_Word,
    x18: seL4_Word,
    x29: seL4_Word,
    x30: seL4_Word,
    x9: seL4_Word,
    x10: seL4_Word,
    x11: seL4_Word,
    x12: seL4_Word,
    x13: seL4_Word,
    x14: seL4_Word,
    x15: seL4_Word,
    x19: seL4_Word,
    x20: seL4_Word,
    x21: seL4_Word,
    x22: seL4_Word,
    x23: seL4_Word,
    x24: seL4_Word,
    x25: seL4_Word,
    x26: seL4_Word,
    x27: seL4_Word,
    x28: seL4_Word,
    tpidr_el0: seL4_Word,
    tpidrro_el0: seL4_Word,
};
pub const seL4_UserContext = struct_seL4_UserContext_;
pub const seL4_ARM_Page = seL4_CPtr;
pub const seL4_ARM_PageTable = seL4_CPtr;
pub const seL4_ARM_PageDirectory = seL4_CPtr;
pub const seL4_ARM_ASIDControl = seL4_CPtr;
pub const seL4_ARM_ASIDPool = seL4_CPtr;
pub const seL4_ARM_VCPU = seL4_CPtr;
pub const seL4_ARM_IOSpace = seL4_CPtr;
pub const seL4_ARM_IOPageTable = seL4_CPtr;
pub const seL4_ARM_SIDControl = seL4_CPtr;
pub const seL4_ARM_SID = seL4_CPtr;
pub const seL4_ARM_CBControl = seL4_CPtr;
pub const seL4_ARM_CB = seL4_CPtr;
pub const seL4_ARM_PageCacheable: c_int = 1;
pub const seL4_ARM_ParityEnabled: c_int = 2;
pub const seL4_ARM_Default_VMAttributes: c_int = 3;
pub const seL4_ARM_ExecuteNever: c_int = 4;
pub const _enum_pad_seL4_ARM_VMAttributes: c_ulong = 9223372036854775807;
pub const seL4_ARM_VMAttributes = c_ulong;
pub const seL4_ARM_CacheI: c_int = 1;
pub const seL4_ARM_CacheD: c_int = 2;
pub const seL4_ARM_CacheID: c_int = 3;
pub const _enum_pad_seL4_ARM_CacheType: c_ulong = 9223372036854775807;
pub const seL4_ARM_CacheType = c_ulong;
pub extern fn __assert_fail(str: [*c]const u8, file: [*c]const u8, line: c_int, function: [*c]const u8) void;
pub const struct_seL4_Fault = extern struct {
    words: [14]seL4_Uint64,
};
pub const seL4_Fault_t = struct_seL4_Fault;
pub const seL4_Fault_NullFault: c_int = 0;
pub const seL4_Fault_CapFault: c_int = 1;
pub const seL4_Fault_UnknownSyscall: c_int = 2;
pub const seL4_Fault_UserException: c_int = 3;
pub const seL4_Fault_Timeout: c_int = 5;
pub const seL4_Fault_VMFault: c_int = 6;
pub const seL4_Fault_VGICMaintenance: c_int = 7;
pub const seL4_Fault_VCPUFault: c_int = 8;
pub const seL4_Fault_VPPIEvent: c_int = 9;
pub const enum_seL4_Fault_tag = c_uint;
pub const seL4_Fault_tag_t = enum_seL4_Fault_tag;
pub fn seL4_Fault_get_seL4_FaultType(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    return @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)))) & @as(c_ulonglong, 15)))));
}
pub fn seL4_Fault_seL4_FaultType_equals(arg_seL4_Fault_1: seL4_Fault_t, arg_seL4_Fault_type_tag: seL4_Uint64) callconv(.C) c_int {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var seL4_Fault_type_tag = arg_seL4_Fault_type_tag;
    return @intFromBool((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)))) & @as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_type_tag))));
}
pub fn seL4_Fault_ptr_get_seL4_FaultType(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    return @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)))) & @as(c_ulonglong, 15)))));
}
pub fn seL4_Fault_NullFault_new() callconv(.C) seL4_Fault_t {
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_NullFault)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_NullFault)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_NullFault & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_NullFault & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 46), "seL4_Fault_NullFault_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_NullFault)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
    return seL4_Fault_1;
}
pub fn seL4_Fault_NullFault_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_NullFault)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_NullFault)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_NullFault & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_NullFault & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 70), "seL4_Fault_NullFault_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_NullFault)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
}
pub fn seL4_Fault_CapFault_new(arg_IP: seL4_Uint64, arg_Addr: seL4_Uint64, arg_InRecvPhase: seL4_Uint64, arg_LookupFailureType: seL4_Uint64, arg_MR4: seL4_Uint64, arg_MR5: seL4_Uint64, arg_MR6: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var IP = arg_IP;
    var Addr = arg_Addr;
    var InRecvPhase = arg_InRecvPhase;
    var LookupFailureType = arg_LookupFailureType;
    var MR4 = arg_MR4;
    var MR5 = arg_MR5;
    var MR6 = arg_MR6;
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_CapFault & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_CapFault & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 94), "seL4_Fault_CapFault_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (MR6 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (MR5 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (MR4 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (LookupFailureType << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (InRecvPhase << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Addr << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (IP << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
    return seL4_Fault_1;
}
pub fn seL4_Fault_CapFault_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_IP: seL4_Uint64, arg_Addr: seL4_Uint64, arg_InRecvPhase: seL4_Uint64, arg_LookupFailureType: seL4_Uint64, arg_MR4: seL4_Uint64, arg_MR5: seL4_Uint64, arg_MR6: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var IP = arg_IP;
    var Addr = arg_Addr;
    var InRecvPhase = arg_InRecvPhase;
    var LookupFailureType = arg_LookupFailureType;
    var MR4 = arg_MR4;
    var MR5 = arg_MR5;
    var MR6 = arg_MR6;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_CapFault & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_CapFault & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 125), "seL4_Fault_CapFault_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (MR6 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (MR5 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (MR4 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (LookupFailureType << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (InRecvPhase << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Addr << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (IP << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
}
pub fn seL4_Fault_CapFault_get_IP(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 155), "seL4_Fault_CapFault_get_IP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_set_IP(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 168), "seL4_Fault_CapFault_set_IP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 170), "seL4_Fault_CapFault_set_IP");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_CapFault_ptr_get_IP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 181), "seL4_Fault_CapFault_ptr_get_IP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_ptr_set_IP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 195), "seL4_Fault_CapFault_ptr_set_IP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 198), "seL4_Fault_CapFault_ptr_set_IP");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_CapFault_get_Addr(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 208), "seL4_Fault_CapFault_get_Addr");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_set_Addr(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 221), "seL4_Fault_CapFault_set_Addr");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 223), "seL4_Fault_CapFault_set_Addr");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_CapFault_ptr_get_Addr(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 234), "seL4_Fault_CapFault_ptr_get_Addr");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_ptr_set_Addr(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 248), "seL4_Fault_CapFault_ptr_set_Addr");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 251), "seL4_Fault_CapFault_ptr_set_Addr");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_CapFault_get_InRecvPhase(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 261), "seL4_Fault_CapFault_get_InRecvPhase");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_set_InRecvPhase(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 274), "seL4_Fault_CapFault_set_InRecvPhase");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 276), "seL4_Fault_CapFault_set_InRecvPhase");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_CapFault_ptr_get_InRecvPhase(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 287), "seL4_Fault_CapFault_ptr_get_InRecvPhase");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_ptr_set_InRecvPhase(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 301), "seL4_Fault_CapFault_ptr_set_InRecvPhase");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 304), "seL4_Fault_CapFault_ptr_set_InRecvPhase");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_CapFault_get_LookupFailureType(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 314), "seL4_Fault_CapFault_get_LookupFailureType");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_set_LookupFailureType(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 327), "seL4_Fault_CapFault_set_LookupFailureType");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 329), "seL4_Fault_CapFault_set_LookupFailureType");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_CapFault_ptr_get_LookupFailureType(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 340), "seL4_Fault_CapFault_ptr_get_LookupFailureType");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_ptr_set_LookupFailureType(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 354), "seL4_Fault_CapFault_ptr_set_LookupFailureType");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 357), "seL4_Fault_CapFault_ptr_set_LookupFailureType");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_CapFault_get_MR4(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 367), "seL4_Fault_CapFault_get_MR4");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_set_MR4(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 380), "seL4_Fault_CapFault_set_MR4");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 382), "seL4_Fault_CapFault_set_MR4");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_CapFault_ptr_get_MR4(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 393), "seL4_Fault_CapFault_ptr_get_MR4");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_ptr_set_MR4(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 407), "seL4_Fault_CapFault_ptr_set_MR4");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 410), "seL4_Fault_CapFault_ptr_set_MR4");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_CapFault_get_MR5(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 420), "seL4_Fault_CapFault_get_MR5");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_set_MR5(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 433), "seL4_Fault_CapFault_set_MR5");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 435), "seL4_Fault_CapFault_set_MR5");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_CapFault_ptr_get_MR5(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 446), "seL4_Fault_CapFault_ptr_get_MR5");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_ptr_set_MR5(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 460), "seL4_Fault_CapFault_ptr_set_MR5");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 463), "seL4_Fault_CapFault_ptr_set_MR5");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_CapFault_get_MR6(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 473), "seL4_Fault_CapFault_get_MR6");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_set_MR6(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 486), "seL4_Fault_CapFault_set_MR6");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 488), "seL4_Fault_CapFault_set_MR6");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_CapFault_ptr_get_MR6(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 499), "seL4_Fault_CapFault_ptr_get_MR6");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_CapFault_ptr_set_MR6(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_CapFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 513), "seL4_Fault_CapFault_ptr_set_MR6");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 516), "seL4_Fault_CapFault_ptr_set_MR6");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_new(arg_X0: seL4_Uint64, arg_X1: seL4_Uint64, arg_X2: seL4_Uint64, arg_X3: seL4_Uint64, arg_X4: seL4_Uint64, arg_X5: seL4_Uint64, arg_X6: seL4_Uint64, arg_X7: seL4_Uint64, arg_FaultIP: seL4_Uint64, arg_SP: seL4_Uint64, arg_LR: seL4_Uint64, arg_SPSR: seL4_Uint64, arg_Syscall: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var X0 = arg_X0;
    var X1 = arg_X1;
    var X2 = arg_X2;
    var X3 = arg_X3;
    var X4 = arg_X4;
    var X5 = arg_X5;
    var X6 = arg_X6;
    var X7 = arg_X7;
    var FaultIP = arg_FaultIP;
    var SP = arg_SP;
    var LR = arg_LR;
    var SPSR = arg_SPSR;
    var Syscall = arg_Syscall;
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_UnknownSyscall & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_UnknownSyscall & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 527), "seL4_Fault_UnknownSyscall_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Syscall << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (SPSR << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (LR << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (SP << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (FaultIP << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X7 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X6 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X5 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X4 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X3 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X2 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X1 << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X0 << @intCast(0));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_X0: seL4_Uint64, arg_X1: seL4_Uint64, arg_X2: seL4_Uint64, arg_X3: seL4_Uint64, arg_X4: seL4_Uint64, arg_X5: seL4_Uint64, arg_X6: seL4_Uint64, arg_X7: seL4_Uint64, arg_FaultIP: seL4_Uint64, arg_SP: seL4_Uint64, arg_LR: seL4_Uint64, arg_SPSR: seL4_Uint64, arg_Syscall: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var X0 = arg_X0;
    var X1 = arg_X1;
    var X2 = arg_X2;
    var X3 = arg_X3;
    var X4 = arg_X4;
    var X5 = arg_X5;
    var X6 = arg_X6;
    var X7 = arg_X7;
    var FaultIP = arg_FaultIP;
    var SP = arg_SP;
    var LR = arg_LR;
    var SPSR = arg_SPSR;
    var Syscall = arg_Syscall;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_UnknownSyscall & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_UnknownSyscall & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 564), "seL4_Fault_UnknownSyscall_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Syscall << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (SPSR << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (LR << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (SP << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (FaultIP << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X7 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X6 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X5 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X4 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X3 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X2 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X1 << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (X0 << @intCast(0));
}
pub fn seL4_Fault_UnknownSyscall_get_X0(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 600), "seL4_Fault_UnknownSyscall_get_X0");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_X0(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 613), "seL4_Fault_UnknownSyscall_set_X0");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 615), "seL4_Fault_UnknownSyscall_set_X0");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_X0(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 626), "seL4_Fault_UnknownSyscall_ptr_get_X0");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_X0(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 640), "seL4_Fault_UnknownSyscall_ptr_set_X0");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 643), "seL4_Fault_UnknownSyscall_ptr_set_X0");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_X1(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 653), "seL4_Fault_UnknownSyscall_get_X1");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_X1(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 666), "seL4_Fault_UnknownSyscall_set_X1");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 668), "seL4_Fault_UnknownSyscall_set_X1");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_X1(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 679), "seL4_Fault_UnknownSyscall_ptr_get_X1");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_X1(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 693), "seL4_Fault_UnknownSyscall_ptr_set_X1");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 696), "seL4_Fault_UnknownSyscall_ptr_set_X1");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_X2(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 706), "seL4_Fault_UnknownSyscall_get_X2");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_X2(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 719), "seL4_Fault_UnknownSyscall_set_X2");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 721), "seL4_Fault_UnknownSyscall_set_X2");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_X2(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 732), "seL4_Fault_UnknownSyscall_ptr_get_X2");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_X2(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 746), "seL4_Fault_UnknownSyscall_ptr_set_X2");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 749), "seL4_Fault_UnknownSyscall_ptr_set_X2");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_X3(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 759), "seL4_Fault_UnknownSyscall_get_X3");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_X3(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 772), "seL4_Fault_UnknownSyscall_set_X3");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 774), "seL4_Fault_UnknownSyscall_set_X3");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_X3(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 785), "seL4_Fault_UnknownSyscall_ptr_get_X3");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_X3(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 799), "seL4_Fault_UnknownSyscall_ptr_set_X3");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 802), "seL4_Fault_UnknownSyscall_ptr_set_X3");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_X4(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 812), "seL4_Fault_UnknownSyscall_get_X4");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_X4(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 825), "seL4_Fault_UnknownSyscall_set_X4");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 827), "seL4_Fault_UnknownSyscall_set_X4");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_X4(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 838), "seL4_Fault_UnknownSyscall_ptr_get_X4");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_X4(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 852), "seL4_Fault_UnknownSyscall_ptr_set_X4");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 855), "seL4_Fault_UnknownSyscall_ptr_set_X4");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_X5(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 865), "seL4_Fault_UnknownSyscall_get_X5");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_X5(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 878), "seL4_Fault_UnknownSyscall_set_X5");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 880), "seL4_Fault_UnknownSyscall_set_X5");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_X5(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 891), "seL4_Fault_UnknownSyscall_ptr_get_X5");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_X5(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 905), "seL4_Fault_UnknownSyscall_ptr_set_X5");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 908), "seL4_Fault_UnknownSyscall_ptr_set_X5");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_X6(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 918), "seL4_Fault_UnknownSyscall_get_X6");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_X6(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 931), "seL4_Fault_UnknownSyscall_set_X6");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 933), "seL4_Fault_UnknownSyscall_set_X6");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_X6(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 944), "seL4_Fault_UnknownSyscall_ptr_get_X6");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_X6(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 958), "seL4_Fault_UnknownSyscall_ptr_set_X6");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 961), "seL4_Fault_UnknownSyscall_ptr_set_X6");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_X7(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 971), "seL4_Fault_UnknownSyscall_get_X7");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_X7(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 984), "seL4_Fault_UnknownSyscall_set_X7");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 986), "seL4_Fault_UnknownSyscall_set_X7");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_X7(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 997), "seL4_Fault_UnknownSyscall_ptr_get_X7");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_X7(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1011), "seL4_Fault_UnknownSyscall_ptr_set_X7");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1014), "seL4_Fault_UnknownSyscall_ptr_set_X7");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_FaultIP(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1024), "seL4_Fault_UnknownSyscall_get_FaultIP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_FaultIP(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1037), "seL4_Fault_UnknownSyscall_set_FaultIP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1039), "seL4_Fault_UnknownSyscall_set_FaultIP");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_FaultIP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1050), "seL4_Fault_UnknownSyscall_ptr_get_FaultIP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_FaultIP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1064), "seL4_Fault_UnknownSyscall_ptr_set_FaultIP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1067), "seL4_Fault_UnknownSyscall_ptr_set_FaultIP");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_SP(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1077), "seL4_Fault_UnknownSyscall_get_SP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_SP(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1090), "seL4_Fault_UnknownSyscall_set_SP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1092), "seL4_Fault_UnknownSyscall_set_SP");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_SP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1103), "seL4_Fault_UnknownSyscall_ptr_get_SP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_SP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1117), "seL4_Fault_UnknownSyscall_ptr_set_SP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1120), "seL4_Fault_UnknownSyscall_ptr_set_SP");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_LR(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1130), "seL4_Fault_UnknownSyscall_get_LR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_LR(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1143), "seL4_Fault_UnknownSyscall_set_LR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1145), "seL4_Fault_UnknownSyscall_set_LR");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_LR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1156), "seL4_Fault_UnknownSyscall_ptr_get_LR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_LR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1170), "seL4_Fault_UnknownSyscall_ptr_set_LR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1173), "seL4_Fault_UnknownSyscall_ptr_set_LR");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_SPSR(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1183), "seL4_Fault_UnknownSyscall_get_SPSR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_SPSR(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1196), "seL4_Fault_UnknownSyscall_set_SPSR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1198), "seL4_Fault_UnknownSyscall_set_SPSR");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_SPSR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1209), "seL4_Fault_UnknownSyscall_ptr_get_SPSR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_SPSR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1223), "seL4_Fault_UnknownSyscall_ptr_set_SPSR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1226), "seL4_Fault_UnknownSyscall_ptr_set_SPSR");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UnknownSyscall_get_Syscall(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1236), "seL4_Fault_UnknownSyscall_get_Syscall");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_set_Syscall(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1249), "seL4_Fault_UnknownSyscall_set_Syscall");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1251), "seL4_Fault_UnknownSyscall_set_Syscall");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UnknownSyscall_ptr_get_Syscall(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1262), "seL4_Fault_UnknownSyscall_ptr_get_Syscall");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UnknownSyscall_ptr_set_Syscall(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UnknownSyscall", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1276), "seL4_Fault_UnknownSyscall_ptr_set_Syscall");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1279), "seL4_Fault_UnknownSyscall_ptr_set_Syscall");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UserException_new(arg_FaultIP: seL4_Uint64, arg_Stack: seL4_Uint64, arg_SPSR: seL4_Uint64, arg_Number: seL4_Uint64, arg_Code: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var FaultIP = arg_FaultIP;
    var Stack = arg_Stack;
    var SPSR = arg_SPSR;
    var Number = arg_Number;
    var Code = arg_Code;
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_UserException & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_UserException & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1290), "seL4_Fault_UserException_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Code << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Number << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (SPSR << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Stack << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (FaultIP << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
    return seL4_Fault_1;
}
pub fn seL4_Fault_UserException_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_FaultIP: seL4_Uint64, arg_Stack: seL4_Uint64, arg_SPSR: seL4_Uint64, arg_Number: seL4_Uint64, arg_Code: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var FaultIP = arg_FaultIP;
    var Stack = arg_Stack;
    var SPSR = arg_SPSR;
    var Number = arg_Number;
    var Code = arg_Code;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_UserException & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_UserException & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1319), "seL4_Fault_UserException_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Code << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Number << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (SPSR << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Stack << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (FaultIP << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
}
pub fn seL4_Fault_UserException_get_FaultIP(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1347), "seL4_Fault_UserException_get_FaultIP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_set_FaultIP(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1360), "seL4_Fault_UserException_set_FaultIP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1362), "seL4_Fault_UserException_set_FaultIP");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UserException_ptr_get_FaultIP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1373), "seL4_Fault_UserException_ptr_get_FaultIP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_ptr_set_FaultIP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1387), "seL4_Fault_UserException_ptr_set_FaultIP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1390), "seL4_Fault_UserException_ptr_set_FaultIP");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UserException_get_Stack(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1400), "seL4_Fault_UserException_get_Stack");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_set_Stack(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1413), "seL4_Fault_UserException_set_Stack");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1415), "seL4_Fault_UserException_set_Stack");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UserException_ptr_get_Stack(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1426), "seL4_Fault_UserException_ptr_get_Stack");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_ptr_set_Stack(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1440), "seL4_Fault_UserException_ptr_set_Stack");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1443), "seL4_Fault_UserException_ptr_set_Stack");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UserException_get_SPSR(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1453), "seL4_Fault_UserException_get_SPSR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_set_SPSR(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1466), "seL4_Fault_UserException_set_SPSR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1468), "seL4_Fault_UserException_set_SPSR");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UserException_ptr_get_SPSR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1479), "seL4_Fault_UserException_ptr_get_SPSR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_ptr_set_SPSR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1493), "seL4_Fault_UserException_ptr_set_SPSR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1496), "seL4_Fault_UserException_ptr_set_SPSR");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UserException_get_Number(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1506), "seL4_Fault_UserException_get_Number");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_set_Number(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1519), "seL4_Fault_UserException_set_Number");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1521), "seL4_Fault_UserException_set_Number");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UserException_ptr_get_Number(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1532), "seL4_Fault_UserException_ptr_get_Number");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_ptr_set_Number(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1546), "seL4_Fault_UserException_ptr_set_Number");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1549), "seL4_Fault_UserException_ptr_set_Number");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_UserException_get_Code(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1559), "seL4_Fault_UserException_get_Code");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_set_Code(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1572), "seL4_Fault_UserException_set_Code");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1574), "seL4_Fault_UserException_set_Code");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_UserException_ptr_get_Code(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1585), "seL4_Fault_UserException_ptr_get_Code");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_UserException_ptr_set_Code(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_UserException", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1599), "seL4_Fault_UserException_ptr_set_Code");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1602), "seL4_Fault_UserException_ptr_set_Code");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_Timeout_new(arg_data: seL4_Uint64, arg_consumed: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var data = arg_data;
    var consumed = arg_consumed;
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_Timeout & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_Timeout & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1613), "seL4_Fault_Timeout_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (consumed << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (data << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
    return seL4_Fault_1;
}
pub fn seL4_Fault_Timeout_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_data: seL4_Uint64, arg_consumed: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var data = arg_data;
    var consumed = arg_consumed;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_Timeout & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_Timeout & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1639), "seL4_Fault_Timeout_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (consumed << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (data << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
}
pub fn seL4_Fault_Timeout_get_data(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_Timeout", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1664), "seL4_Fault_Timeout_get_data");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_Timeout_set_data(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_Timeout", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1677), "seL4_Fault_Timeout_set_data");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1679), "seL4_Fault_Timeout_set_data");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_Timeout_ptr_get_data(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_Timeout", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1690), "seL4_Fault_Timeout_ptr_get_data");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_Timeout_ptr_set_data(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_Timeout", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1704), "seL4_Fault_Timeout_ptr_set_data");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1707), "seL4_Fault_Timeout_ptr_set_data");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_Timeout_get_consumed(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_Timeout", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1717), "seL4_Fault_Timeout_get_consumed");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_Timeout_set_consumed(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_Timeout", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1730), "seL4_Fault_Timeout_set_consumed");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1732), "seL4_Fault_Timeout_set_consumed");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_Timeout_ptr_get_consumed(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_Timeout", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1743), "seL4_Fault_Timeout_ptr_get_consumed");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_Timeout_ptr_set_consumed(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_Timeout", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1757), "seL4_Fault_Timeout_ptr_set_consumed");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1760), "seL4_Fault_Timeout_ptr_set_consumed");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_VMFault_new(arg_IP: seL4_Uint64, arg_Addr: seL4_Uint64, arg_PrefetchFault: seL4_Uint64, arg_FSR: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var IP = arg_IP;
    var Addr = arg_Addr;
    var PrefetchFault = arg_PrefetchFault;
    var FSR = arg_FSR;
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_VMFault & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_VMFault & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1771), "seL4_Fault_VMFault_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (FSR << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (PrefetchFault << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Addr << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (IP << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
    return seL4_Fault_1;
}
pub fn seL4_Fault_VMFault_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_IP: seL4_Uint64, arg_Addr: seL4_Uint64, arg_PrefetchFault: seL4_Uint64, arg_FSR: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var IP = arg_IP;
    var Addr = arg_Addr;
    var PrefetchFault = arg_PrefetchFault;
    var FSR = arg_FSR;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_VMFault & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_VMFault & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1799), "seL4_Fault_VMFault_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (FSR << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (PrefetchFault << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (Addr << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (IP << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
}
pub fn seL4_Fault_VMFault_get_IP(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1826), "seL4_Fault_VMFault_get_IP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VMFault_set_IP(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1839), "seL4_Fault_VMFault_set_IP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1841), "seL4_Fault_VMFault_set_IP");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_VMFault_ptr_get_IP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1852), "seL4_Fault_VMFault_ptr_get_IP");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VMFault_ptr_set_IP(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1866), "seL4_Fault_VMFault_ptr_set_IP");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1869), "seL4_Fault_VMFault_ptr_set_IP");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_VMFault_get_Addr(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1879), "seL4_Fault_VMFault_get_Addr");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VMFault_set_Addr(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1892), "seL4_Fault_VMFault_set_Addr");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1894), "seL4_Fault_VMFault_set_Addr");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_VMFault_ptr_get_Addr(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1905), "seL4_Fault_VMFault_ptr_get_Addr");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VMFault_ptr_set_Addr(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1919), "seL4_Fault_VMFault_ptr_set_Addr");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1922), "seL4_Fault_VMFault_ptr_set_Addr");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_VMFault_get_PrefetchFault(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1932), "seL4_Fault_VMFault_get_PrefetchFault");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VMFault_set_PrefetchFault(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1945), "seL4_Fault_VMFault_set_PrefetchFault");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1947), "seL4_Fault_VMFault_set_PrefetchFault");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_VMFault_ptr_get_PrefetchFault(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1958), "seL4_Fault_VMFault_ptr_get_PrefetchFault");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VMFault_ptr_set_PrefetchFault(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1972), "seL4_Fault_VMFault_ptr_set_PrefetchFault");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1975), "seL4_Fault_VMFault_ptr_set_PrefetchFault");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_VMFault_get_FSR(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1985), "seL4_Fault_VMFault_get_FSR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VMFault_set_FSR(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 1998), "seL4_Fault_VMFault_set_FSR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2000), "seL4_Fault_VMFault_set_FSR");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_VMFault_ptr_get_FSR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2011), "seL4_Fault_VMFault_ptr_get_FSR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VMFault_ptr_set_FSR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VMFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2025), "seL4_Fault_VMFault_ptr_set_FSR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2028), "seL4_Fault_VMFault_ptr_set_FSR");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_VGICMaintenance_new(arg_IDX: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var IDX = arg_IDX;
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_VGICMaintenance & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_VGICMaintenance & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2039), "seL4_Fault_VGICMaintenance_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (IDX << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
    return seL4_Fault_1;
}
pub fn seL4_Fault_VGICMaintenance_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_IDX: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var IDX = arg_IDX;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_VGICMaintenance & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_VGICMaintenance & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2064), "seL4_Fault_VGICMaintenance_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (IDX << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
}
pub fn seL4_Fault_VGICMaintenance_get_IDX(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VGICMaintenance", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2088), "seL4_Fault_VGICMaintenance_get_IDX");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VGICMaintenance_set_IDX(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VGICMaintenance", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2101), "seL4_Fault_VGICMaintenance_set_IDX");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2103), "seL4_Fault_VGICMaintenance_set_IDX");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_VGICMaintenance_ptr_get_IDX(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VGICMaintenance", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2114), "seL4_Fault_VGICMaintenance_ptr_get_IDX");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VGICMaintenance_ptr_set_IDX(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VGICMaintenance))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VGICMaintenance", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2128), "seL4_Fault_VGICMaintenance_ptr_set_IDX");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2131), "seL4_Fault_VGICMaintenance_ptr_set_IDX");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub fn seL4_Fault_VCPUFault_new(arg_HSR: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var HSR = arg_HSR;
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, HSR))) & ~@as(c_ulonglong, 4294967295)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, HSR))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(HSR & ~0xffffffffull) == ((0 && (HSR & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2142), "seL4_Fault_VCPUFault_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_VCPUFault & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_VCPUFault & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2143), "seL4_Fault_VCPUFault_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, HSR))) & @as(c_ulonglong, 4294967295)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
    return seL4_Fault_1;
}
pub fn seL4_Fault_VCPUFault_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_HSR: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var HSR = arg_HSR;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, HSR))) & ~@as(c_ulonglong, 4294967295)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, HSR))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(HSR & ~0xffffffffull) == ((0 && (HSR & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2168), "seL4_Fault_VCPUFault_ptr_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_VCPUFault & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_VCPUFault & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2169), "seL4_Fault_VCPUFault_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, HSR))) & @as(c_ulonglong, 4294967295)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
}
pub fn seL4_Fault_VCPUFault_get_HSR(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VCPUFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2193), "seL4_Fault_VCPUFault_get_HSR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 4294967295)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VCPUFault_set_HSR(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VCPUFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2206), "seL4_Fault_VCPUFault_set_HSR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 4294967295) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2208), "seL4_Fault_VCPUFault_set_HSR");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 4294967295)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 4294967295)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_VCPUFault_ptr_get_HSR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VCPUFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2219), "seL4_Fault_VCPUFault_ptr_get_HSR");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 4294967295)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VCPUFault_ptr_set_HSR(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VCPUFault))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VCPUFault", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2233), "seL4_Fault_VCPUFault_ptr_set_HSR");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 4294967295) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2236), "seL4_Fault_VCPUFault_ptr_set_HSR");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 4294967295)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 4294967295)))));
}
pub fn seL4_Fault_VPPIEvent_new(arg_irq: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var irq = arg_irq;
    var seL4_Fault_1: seL4_Fault_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_VPPIEvent & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_VPPIEvent & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2247), "seL4_Fault_VPPIEvent_new");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (irq << @intCast(0));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 2)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
    return seL4_Fault_1;
}
pub fn seL4_Fault_VPPIEvent_ptr_new(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_irq: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var irq = arg_irq;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent)))))) & ~@as(c_ulonglong, 15)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent)))))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("((seL4_Uint64)seL4_Fault_VPPIEvent & ~0xfull) == ((0 && ((seL4_Uint64)seL4_Fault_VPPIEvent & (1ull << 63))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2272), "seL4_Fault_VPPIEvent_ptr_new");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent)))))) & @as(c_ulonglong, 15)) << @intCast(0))))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] = @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))) | (irq << @intCast(0));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 2)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 3)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 4)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 5)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 6)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 7)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 8)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 9)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 10)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 11)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 12)))] = 0;
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 13)))] = 0;
}
pub fn seL4_Fault_VPPIEvent_get_irq(arg_seL4_Fault_1: seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VPPIEvent", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2296), "seL4_Fault_VPPIEvent_get_irq");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VPPIEvent_set_irq(arg_seL4_Fault_1: seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) seL4_Fault_t {
    var seL4_Fault_1 = arg_seL4_Fault_1;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent))))) {
            __assert_fail("((seL4_Fault.words[0] >> 0) & 0xf) == seL4_Fault_VPPIEvent", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2309), "seL4_Fault_VPPIEvent_set_irq");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2311), "seL4_Fault_VPPIEvent_set_irq");
        }
        if (!false) break;
    }
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_1.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
    return seL4_Fault_1;
}
pub fn seL4_Fault_VPPIEvent_ptr_get_irq(arg_seL4_Fault_ptr: [*c]seL4_Fault_t) callconv(.C) seL4_Uint64 {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var ret: seL4_Uint64 = undefined;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VPPIEvent", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2322), "seL4_Fault_VPPIEvent_ptr_get_irq");
        }
        if (!false) break;
    }
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))]))) & @as(c_ulonglong, 18446744073709551615)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_Fault_VPPIEvent_ptr_set_irq(arg_seL4_Fault_ptr: [*c]seL4_Fault_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_Fault_ptr = arg_seL4_Fault_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!(((seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] >> @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 15))))) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VPPIEvent))))) {
            __assert_fail("((seL4_Fault_ptr->words[0] >> 0) & 0xf) == seL4_Fault_VPPIEvent", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2336), "seL4_Fault_VPPIEvent_ptr_set_irq");
        }
        if (!false) break;
    }
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551615) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffffull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/sel4_arch/types_gen.h", @as(c_int, 2339), "seL4_Fault_VPPIEvent_ptr_set_irq");
        }
        if (!false) break;
    }
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551615)))));
    seL4_Fault_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 1)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 18446744073709551615)))));
}
pub const seL4_SysCall: c_int = -1;
pub const seL4_SysReplyRecv: c_int = -2;
pub const seL4_SysNBSendRecv: c_int = -3;
pub const seL4_SysNBSendWait: c_int = -4;
pub const seL4_SysSend: c_int = -5;
pub const seL4_SysNBSend: c_int = -6;
pub const seL4_SysRecv: c_int = -7;
pub const seL4_SysNBRecv: c_int = -8;
pub const seL4_SysWait: c_int = -9;
pub const seL4_SysNBWait: c_int = -10;
pub const seL4_SysYield: c_int = -11;
pub const seL4_SysDebugPutChar: c_int = -12;
pub const seL4_SysDebugDumpScheduler: c_int = -13;
pub const seL4_SysDebugHalt: c_int = -14;
pub const seL4_SysDebugCapIdentify: c_int = -15;
pub const seL4_SysDebugSnapshot: c_int = -16;
pub const seL4_SysDebugNameThread: c_int = -17;
pub const _enum_pad_seL4_Syscall_ID: c_long = 9223372036854775807;
pub const seL4_Syscall_ID = c_long;
pub const seL4_UntypedObject: c_int = 0;
pub const seL4_TCBObject: c_int = 1;
pub const seL4_EndpointObject: c_int = 2;
pub const seL4_NotificationObject: c_int = 3;
pub const seL4_CapTableObject: c_int = 4;
pub const seL4_SchedContextObject: c_int = 5;
pub const seL4_ReplyObject: c_int = 6;
pub const seL4_NonArchObjectTypeCount: c_int = 7;
pub const enum_api_object = c_uint;
pub const seL4_ObjectType = enum_api_object;
pub const seL4_AsyncEndpointObject: seL4_ObjectType = @as(c_uint, @bitCast(seL4_NotificationObject));
pub const api_object_t = seL4_Word;
pub const seL4_ARM_HugePageObject: c_int = 7;
pub const seL4_ARM_PageUpperDirectoryObject: c_int = 8;
pub const seL4_ARM_PageGlobalDirectoryObject: c_int = 9;
pub const seL4_ModeObjectTypeCount: c_int = 10;
pub const enum__mode_object = c_uint;
pub const seL4_ModeObjectType = enum__mode_object;
pub const seL4_ARM_SmallPageObject: c_int = 10;
pub const seL4_ARM_LargePageObject: c_int = 11;
pub const seL4_ARM_PageTableObject: c_int = 12;
pub const seL4_ARM_PageDirectoryObject: c_int = 13;
pub const seL4_ARM_VCPUObject: c_int = 14;
pub const seL4_ObjectTypeCount: c_int = 15;
pub const enum__object = c_uint;
pub const seL4_ArchObjectType = enum__object;
pub const object_t = seL4_Word;
pub const seL4_NoError: c_int = 0;
pub const seL4_InvalidArgument: c_int = 1;
pub const seL4_InvalidCapability: c_int = 2;
pub const seL4_IllegalOperation: c_int = 3;
pub const seL4_RangeError: c_int = 4;
pub const seL4_AlignmentError: c_int = 5;
pub const seL4_FailedLookup: c_int = 6;
pub const seL4_TruncatedMessage: c_int = 7;
pub const seL4_DeleteFirst: c_int = 8;
pub const seL4_RevokeFirst: c_int = 9;
pub const seL4_NotEnoughMemory: c_int = 10;
pub const seL4_NumErrors: c_int = 11;
pub const seL4_Error = c_uint;
pub const seL4_InvalidPrio: c_int = -1;
pub const seL4_MinPrio: c_int = 0;
pub const seL4_MaxPrio: c_int = 255;
pub const enum_priorityConstants = c_int;
pub const seL4_MsgLengthBits: c_int = 7;
pub const seL4_MsgExtraCapBits: c_int = 2;
pub const enum_seL4_MsgLimits = c_uint;
pub const seL4_MsgMaxLength: c_int = 120;
const enum_unnamed_1 = c_uint;
pub const seL4_NoFailure: c_int = 0;
pub const seL4_InvalidRoot: c_int = 1;
pub const seL4_MissingCapability: c_int = 2;
pub const seL4_DepthMismatch: c_int = 3;
pub const seL4_GuardMismatch: c_int = 4;
pub const _enum_pad_seL4_LookupFailureType: c_ulong = 9223372036854775807;
pub const seL4_LookupFailureType = c_ulong; // ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
// ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
pub fn seL4_MaxExtraRefills(arg_size: seL4_Word) callconv(.C) seL4_Word {
    var size = arg_size;
    return ((@as(c_ulong, 1) << @intCast(size)) -% ((@as(c_ulong, @bitCast(@as(c_long, @as(c_int, 10)))) *% @sizeOf(seL4_Word)) +% @as(c_ulong, @bitCast(@as(c_long, @as(c_int, 6) * @as(c_int, 8)))))) / @as(c_ulong, @bitCast(@as(c_long, @as(c_int, 2) * @as(c_int, 8))));
}
pub const seL4_SchedContext_NoFlag: c_int = 0;
pub const seL4_SchedContext_Sporadic: c_int = 1;
pub const _enum_pad_seL4_SchedContextFlag: c_ulong = 9223372036854775807;
pub const seL4_SchedContextFlag = c_ulong;
pub const struct_seL4_CNode_CapData = extern struct {
    words: [1]seL4_Uint64,
};
pub const seL4_CNode_CapData_t = struct_seL4_CNode_CapData;
pub fn seL4_CNode_CapData_new(arg_guard: seL4_Uint64, arg_guardSize: seL4_Uint64) callconv(.C) seL4_CNode_CapData_t {
    var guard = arg_guard;
    var guardSize = arg_guardSize;
    var seL4_CNode_CapData_1: seL4_CNode_CapData_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guard))) & ~@as(c_ulonglong, 288230376151711743)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guard))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(guard & ~0x3ffffffffffffffull) == ((0 && (guard & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 18), "seL4_CNode_CapData_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guardSize))) & ~@as(c_ulonglong, 63)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guardSize))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(guardSize & ~0x3full) == ((0 && (guardSize & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 19), "seL4_CNode_CapData_new");
        }
        if (!false) break;
    }
    seL4_CNode_CapData_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guard))) & @as(c_ulonglong, 288230376151711743)) << @intCast(6))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guardSize))) & @as(c_ulonglong, 63)) << @intCast(0))))));
    return seL4_CNode_CapData_1;
}
pub fn seL4_CNode_CapData_ptr_new(arg_seL4_CNode_CapData_ptr: [*c]seL4_CNode_CapData_t, arg_guard: seL4_Uint64, arg_guardSize: seL4_Uint64) callconv(.C) void {
    var seL4_CNode_CapData_ptr = arg_seL4_CNode_CapData_ptr;
    var guard = arg_guard;
    var guardSize = arg_guardSize;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guard))) & ~@as(c_ulonglong, 288230376151711743)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guard))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(guard & ~0x3ffffffffffffffull) == ((0 && (guard & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 31), "seL4_CNode_CapData_ptr_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guardSize))) & ~@as(c_ulonglong, 63)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guardSize))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(guardSize & ~0x3full) == ((0 && (guardSize & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 32), "seL4_CNode_CapData_ptr_new");
        }
        if (!false) break;
    }
    seL4_CNode_CapData_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guard))) & @as(c_ulonglong, 288230376151711743)) << @intCast(6))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, guardSize))) & @as(c_ulonglong, 63)) << @intCast(0))))));
}
pub fn seL4_CNode_CapData_get_guard(arg_seL4_CNode_CapData_1: seL4_CNode_CapData_t) callconv(.C) seL4_Uint64 {
    var seL4_CNode_CapData_1 = arg_seL4_CNode_CapData_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CNode_CapData_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 18446744073709551552)) >> @intCast(6)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CNode_CapData_set_guard(arg_seL4_CNode_CapData_1: seL4_CNode_CapData_t, arg_v64: seL4_Uint64) callconv(.C) seL4_CNode_CapData_t {
    var seL4_CNode_CapData_1 = arg_seL4_CNode_CapData_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551552) >> @intCast(6)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffc0ull >> 6 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 53), "seL4_CNode_CapData_set_guard");
        }
        if (!false) break;
    }
    seL4_CNode_CapData_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551552)))));
    seL4_CNode_CapData_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(6)))) & @as(c_ulonglong, 18446744073709551552)))));
    return seL4_CNode_CapData_1;
}
pub fn seL4_CNode_CapData_ptr_get_guard(arg_seL4_CNode_CapData_ptr: [*c]seL4_CNode_CapData_t) callconv(.C) seL4_Uint64 {
    var seL4_CNode_CapData_ptr = arg_seL4_CNode_CapData_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CNode_CapData_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 18446744073709551552)) >> @intCast(6)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CNode_CapData_ptr_set_guard(arg_seL4_CNode_CapData_ptr: [*c]seL4_CNode_CapData_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_CNode_CapData_ptr = arg_seL4_CNode_CapData_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709551552) >> @intCast(6)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xffffffffffffffc0ull >> 6) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 73), "seL4_CNode_CapData_ptr_set_guard");
        }
        if (!false) break;
    }
    seL4_CNode_CapData_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709551552)))));
    seL4_CNode_CapData_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(6)) & @as(c_ulong, 18446744073709551552);
}
pub fn seL4_CNode_CapData_get_guardSize(arg_seL4_CNode_CapData_1: seL4_CNode_CapData_t) callconv(.C) seL4_Uint64 {
    var seL4_CNode_CapData_1 = arg_seL4_CNode_CapData_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CNode_CapData_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 63)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CNode_CapData_set_guardSize(arg_seL4_CNode_CapData_1: seL4_CNode_CapData_t, arg_v64: seL4_Uint64) callconv(.C) seL4_CNode_CapData_t {
    var seL4_CNode_CapData_1 = arg_seL4_CNode_CapData_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 63) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x3full >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 92), "seL4_CNode_CapData_set_guardSize");
        }
        if (!false) break;
    }
    seL4_CNode_CapData_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 63)))));
    seL4_CNode_CapData_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 63)))));
    return seL4_CNode_CapData_1;
}
pub fn seL4_CNode_CapData_ptr_get_guardSize(arg_seL4_CNode_CapData_ptr: [*c]seL4_CNode_CapData_t) callconv(.C) seL4_Uint64 {
    var seL4_CNode_CapData_ptr = arg_seL4_CNode_CapData_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CNode_CapData_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 63)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CNode_CapData_ptr_set_guardSize(arg_seL4_CNode_CapData_ptr: [*c]seL4_CNode_CapData_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_CNode_CapData_ptr = arg_seL4_CNode_CapData_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 63) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x3full >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 112), "seL4_CNode_CapData_ptr_set_guardSize");
        }
        if (!false) break;
    }
    seL4_CNode_CapData_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 63)))));
    seL4_CNode_CapData_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 63))));
}
pub const struct_seL4_CapRights = extern struct {
    words: [1]seL4_Uint64,
};
pub const seL4_CapRights_t = struct_seL4_CapRights;
pub fn seL4_CapRights_new(arg_capAllowGrantReply: seL4_Uint64, arg_capAllowGrant: seL4_Uint64, arg_capAllowRead: seL4_Uint64, arg_capAllowWrite: seL4_Uint64) callconv(.C) seL4_CapRights_t {
    var capAllowGrantReply = arg_capAllowGrantReply;
    var capAllowGrant = arg_capAllowGrant;
    var capAllowRead = arg_capAllowRead;
    var capAllowWrite = arg_capAllowWrite;
    var seL4_CapRights_1: seL4_CapRights_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrantReply))) & ~@as(c_ulonglong, 1)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrantReply))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capAllowGrantReply & ~0x1ull) == ((0 && (capAllowGrantReply & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 127), "seL4_CapRights_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrant))) & ~@as(c_ulonglong, 1)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrant))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capAllowGrant & ~0x1ull) == ((0 && (capAllowGrant & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 128), "seL4_CapRights_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowRead))) & ~@as(c_ulonglong, 1)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowRead))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capAllowRead & ~0x1ull) == ((0 && (capAllowRead & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 129), "seL4_CapRights_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowWrite))) & ~@as(c_ulonglong, 1)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowWrite))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capAllowWrite & ~0x1ull) == ((0 && (capAllowWrite & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 130), "seL4_CapRights_new");
        }
        if (!false) break;
    }
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((((@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrantReply))) & @as(c_ulonglong, 1)) << @intCast(3))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrant))) & @as(c_ulonglong, 1)) << @intCast(2))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowRead))) & @as(c_ulonglong, 1)) << @intCast(1))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowWrite))) & @as(c_ulonglong, 1)) << @intCast(0))))));
    return seL4_CapRights_1;
}
pub fn seL4_CapRights_ptr_new(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t, arg_capAllowGrantReply: seL4_Uint64, arg_capAllowGrant: seL4_Uint64, arg_capAllowRead: seL4_Uint64, arg_capAllowWrite: seL4_Uint64) callconv(.C) void {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var capAllowGrantReply = arg_capAllowGrantReply;
    var capAllowGrant = arg_capAllowGrant;
    var capAllowRead = arg_capAllowRead;
    var capAllowWrite = arg_capAllowWrite;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrantReply))) & ~@as(c_ulonglong, 1)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrantReply))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capAllowGrantReply & ~0x1ull) == ((0 && (capAllowGrantReply & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 144), "seL4_CapRights_ptr_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrant))) & ~@as(c_ulonglong, 1)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrant))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capAllowGrant & ~0x1ull) == ((0 && (capAllowGrant & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 145), "seL4_CapRights_ptr_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowRead))) & ~@as(c_ulonglong, 1)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowRead))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capAllowRead & ~0x1ull) == ((0 && (capAllowRead & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 146), "seL4_CapRights_ptr_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowWrite))) & ~@as(c_ulonglong, 1)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowWrite))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capAllowWrite & ~0x1ull) == ((0 && (capAllowWrite & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 147), "seL4_CapRights_ptr_new");
        }
        if (!false) break;
    }
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((((@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrantReply))) & @as(c_ulonglong, 1)) << @intCast(3))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowGrant))) & @as(c_ulonglong, 1)) << @intCast(2))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowRead))) & @as(c_ulonglong, 1)) << @intCast(1))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capAllowWrite))) & @as(c_ulonglong, 1)) << @intCast(0))))));
}
pub fn seL4_CapRights_get_capAllowGrantReply(arg_seL4_CapRights_1: seL4_CapRights_t) callconv(.C) seL4_Uint64 {
    var seL4_CapRights_1 = arg_seL4_CapRights_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 8)) >> @intCast(3)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CapRights_set_capAllowGrantReply(arg_seL4_CapRights_1: seL4_CapRights_t, arg_v64: seL4_Uint64) callconv(.C) seL4_CapRights_t {
    var seL4_CapRights_1 = arg_seL4_CapRights_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 8) >> @intCast(3)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x8ull >> 3 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 170), "seL4_CapRights_set_capAllowGrantReply");
        }
        if (!false) break;
    }
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 8)))));
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(3)))) & @as(c_ulonglong, 8)))));
    return seL4_CapRights_1;
}
pub fn seL4_CapRights_ptr_get_capAllowGrantReply(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t) callconv(.C) seL4_Uint64 {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 8)) >> @intCast(3)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CapRights_ptr_set_capAllowGrantReply(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 8) >> @intCast(3)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x8ull >> 3) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 190), "seL4_CapRights_ptr_set_capAllowGrantReply");
        }
        if (!false) break;
    }
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 8)))));
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(3)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 8))));
}
pub fn seL4_CapRights_get_capAllowGrant(arg_seL4_CapRights_1: seL4_CapRights_t) callconv(.C) seL4_Uint64 {
    var seL4_CapRights_1 = arg_seL4_CapRights_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 4)) >> @intCast(2)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CapRights_set_capAllowGrant(arg_seL4_CapRights_1: seL4_CapRights_t, arg_v64: seL4_Uint64) callconv(.C) seL4_CapRights_t {
    var seL4_CapRights_1 = arg_seL4_CapRights_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 4) >> @intCast(2)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x4ull >> 2 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 209), "seL4_CapRights_set_capAllowGrant");
        }
        if (!false) break;
    }
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 4)))));
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(2)))) & @as(c_ulonglong, 4)))));
    return seL4_CapRights_1;
}
pub fn seL4_CapRights_ptr_get_capAllowGrant(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t) callconv(.C) seL4_Uint64 {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 4)) >> @intCast(2)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CapRights_ptr_set_capAllowGrant(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 4) >> @intCast(2)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x4ull >> 2) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 229), "seL4_CapRights_ptr_set_capAllowGrant");
        }
        if (!false) break;
    }
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 4)))));
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(2)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 4))));
}
pub fn seL4_CapRights_get_capAllowRead(arg_seL4_CapRights_1: seL4_CapRights_t) callconv(.C) seL4_Uint64 {
    var seL4_CapRights_1 = arg_seL4_CapRights_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 2)) >> @intCast(1)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CapRights_set_capAllowRead(arg_seL4_CapRights_1: seL4_CapRights_t, arg_v64: seL4_Uint64) callconv(.C) seL4_CapRights_t {
    var seL4_CapRights_1 = arg_seL4_CapRights_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 2) >> @intCast(1)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x2ull >> 1 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 248), "seL4_CapRights_set_capAllowRead");
        }
        if (!false) break;
    }
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 2)))));
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(1)))) & @as(c_ulonglong, 2)))));
    return seL4_CapRights_1;
}
pub fn seL4_CapRights_ptr_get_capAllowRead(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t) callconv(.C) seL4_Uint64 {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 2)) >> @intCast(1)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CapRights_ptr_set_capAllowRead(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 2) >> @intCast(1)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x2ull >> 1) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 268), "seL4_CapRights_ptr_set_capAllowRead");
        }
        if (!false) break;
    }
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 2)))));
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(1)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2))));
}
pub fn seL4_CapRights_get_capAllowWrite(arg_seL4_CapRights_1: seL4_CapRights_t) callconv(.C) seL4_Uint64 {
    var seL4_CapRights_1 = arg_seL4_CapRights_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 1)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CapRights_set_capAllowWrite(arg_seL4_CapRights_1: seL4_CapRights_t, arg_v64: seL4_Uint64) callconv(.C) seL4_CapRights_t {
    var seL4_CapRights_1 = arg_seL4_CapRights_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 1) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x1ull >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 287), "seL4_CapRights_set_capAllowWrite");
        }
        if (!false) break;
    }
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 1)))));
    seL4_CapRights_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 1)))));
    return seL4_CapRights_1;
}
pub fn seL4_CapRights_ptr_get_capAllowWrite(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t) callconv(.C) seL4_Uint64 {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 1)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_CapRights_ptr_set_capAllowWrite(arg_seL4_CapRights_ptr: [*c]seL4_CapRights_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_CapRights_ptr = arg_seL4_CapRights_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 1) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x1ull >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 307), "seL4_CapRights_ptr_set_capAllowWrite");
        }
        if (!false) break;
    }
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 1)))));
    seL4_CapRights_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1))));
}
pub const struct_seL4_MessageInfo = extern struct {
    words: [1]seL4_Uint64,
};
pub const seL4_MessageInfo_t = struct_seL4_MessageInfo;
pub fn seL4_MessageInfo_new(arg_label: seL4_Uint64, arg_capsUnwrapped: seL4_Uint64, arg_extraCaps: seL4_Uint64, arg_length: seL4_Uint64) callconv(.C) seL4_MessageInfo_t {
    var label = arg_label;
    var capsUnwrapped = arg_capsUnwrapped;
    var extraCaps = arg_extraCaps;
    var length = arg_length;
    var seL4_MessageInfo_1: seL4_MessageInfo_t = undefined;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, label))) & ~@as(c_ulonglong, 4503599627370495)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, label))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(label & ~0xfffffffffffffull) == ((0 && (label & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 322), "seL4_MessageInfo_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capsUnwrapped))) & ~@as(c_ulonglong, 7)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capsUnwrapped))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capsUnwrapped & ~0x7ull) == ((0 && (capsUnwrapped & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 323), "seL4_MessageInfo_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, extraCaps))) & ~@as(c_ulonglong, 3)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, extraCaps))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(extraCaps & ~0x3ull) == ((0 && (extraCaps & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 324), "seL4_MessageInfo_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, length))) & ~@as(c_ulonglong, 127)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, length))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(length & ~0x7full) == ((0 && (length & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 325), "seL4_MessageInfo_new");
        }
        if (!false) break;
    }
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((((@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, label))) & @as(c_ulonglong, 4503599627370495)) << @intCast(12))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capsUnwrapped))) & @as(c_ulonglong, 7)) << @intCast(9))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, extraCaps))) & @as(c_ulonglong, 3)) << @intCast(7))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, length))) & @as(c_ulonglong, 127)) << @intCast(0))))));
    return seL4_MessageInfo_1;
}
pub fn seL4_MessageInfo_ptr_new(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t, arg_label: seL4_Uint64, arg_capsUnwrapped: seL4_Uint64, arg_extraCaps: seL4_Uint64, arg_length: seL4_Uint64) callconv(.C) void {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var label = arg_label;
    var capsUnwrapped = arg_capsUnwrapped;
    var extraCaps = arg_extraCaps;
    var length = arg_length;
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, label))) & ~@as(c_ulonglong, 4503599627370495)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, label))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(label & ~0xfffffffffffffull) == ((0 && (label & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 339), "seL4_MessageInfo_ptr_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capsUnwrapped))) & ~@as(c_ulonglong, 7)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capsUnwrapped))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(capsUnwrapped & ~0x7ull) == ((0 && (capsUnwrapped & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 340), "seL4_MessageInfo_ptr_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, extraCaps))) & ~@as(c_ulonglong, 3)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, extraCaps))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(extraCaps & ~0x3ull) == ((0 && (extraCaps & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 341), "seL4_MessageInfo_ptr_new");
        }
        if (!false) break;
    }
    while (true) {
        if (!((@as(c_ulonglong, @bitCast(@as(c_ulonglong, length))) & ~@as(c_ulonglong, 127)) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, length))) & (@as(c_ulonglong, 1) << @intCast(63))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(length & ~0x7full) == ((0 && (length & (1ull << 63))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 342), "seL4_MessageInfo_ptr_new");
        }
        if (!false) break;
    }
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((((@as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0)))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, label))) & @as(c_ulonglong, 4503599627370495)) << @intCast(12))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, capsUnwrapped))) & @as(c_ulonglong, 7)) << @intCast(9))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, extraCaps))) & @as(c_ulonglong, 3)) << @intCast(7))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, length))) & @as(c_ulonglong, 127)) << @intCast(0))))));
}
pub fn seL4_MessageInfo_get_label(arg_seL4_MessageInfo_1: seL4_MessageInfo_t) callconv(.C) seL4_Uint64 {
    var seL4_MessageInfo_1 = arg_seL4_MessageInfo_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 18446744073709547520)) >> @intCast(12)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_MessageInfo_set_label(arg_seL4_MessageInfo_1: seL4_MessageInfo_t, arg_v64: seL4_Uint64) callconv(.C) seL4_MessageInfo_t {
    var seL4_MessageInfo_1 = arg_seL4_MessageInfo_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709547520) >> @intCast(12)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xfffffffffffff000ull >> 12 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 365), "seL4_MessageInfo_set_label");
        }
        if (!false) break;
    }
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709547520)))));
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(12)))) & @as(c_ulonglong, 18446744073709547520)))));
    return seL4_MessageInfo_1;
}
pub fn seL4_MessageInfo_ptr_get_label(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t) callconv(.C) seL4_Uint64 {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 18446744073709547520)) >> @intCast(12)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_MessageInfo_ptr_set_label(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 18446744073709547520) >> @intCast(12)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xfffffffffffff000ull >> 12) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 385), "seL4_MessageInfo_ptr_set_label");
        }
        if (!false) break;
    }
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 18446744073709547520)))));
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(12)) & @as(c_ulong, 18446744073709547520);
}
pub fn seL4_MessageInfo_get_capsUnwrapped(arg_seL4_MessageInfo_1: seL4_MessageInfo_t) callconv(.C) seL4_Uint64 {
    var seL4_MessageInfo_1 = arg_seL4_MessageInfo_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 3584)) >> @intCast(9)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_MessageInfo_set_capsUnwrapped(arg_seL4_MessageInfo_1: seL4_MessageInfo_t, arg_v64: seL4_Uint64) callconv(.C) seL4_MessageInfo_t {
    var seL4_MessageInfo_1 = arg_seL4_MessageInfo_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 3584) >> @intCast(9)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xe00ull >> 9 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 404), "seL4_MessageInfo_set_capsUnwrapped");
        }
        if (!false) break;
    }
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 3584)))));
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(9)))) & @as(c_ulonglong, 3584)))));
    return seL4_MessageInfo_1;
}
pub fn seL4_MessageInfo_ptr_get_capsUnwrapped(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t) callconv(.C) seL4_Uint64 {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 3584)) >> @intCast(9)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_MessageInfo_ptr_set_capsUnwrapped(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 3584) >> @intCast(9)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0xe00ull >> 9) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 424), "seL4_MessageInfo_ptr_set_capsUnwrapped");
        }
        if (!false) break;
    }
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 3584)))));
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(9)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3584))));
}
pub fn seL4_MessageInfo_get_extraCaps(arg_seL4_MessageInfo_1: seL4_MessageInfo_t) callconv(.C) seL4_Uint64 {
    var seL4_MessageInfo_1 = arg_seL4_MessageInfo_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 384)) >> @intCast(7)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_MessageInfo_set_extraCaps(arg_seL4_MessageInfo_1: seL4_MessageInfo_t, arg_v64: seL4_Uint64) callconv(.C) seL4_MessageInfo_t {
    var seL4_MessageInfo_1 = arg_seL4_MessageInfo_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 384) >> @intCast(7)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x180ull >> 7 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 443), "seL4_MessageInfo_set_extraCaps");
        }
        if (!false) break;
    }
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 384)))));
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(7)))) & @as(c_ulonglong, 384)))));
    return seL4_MessageInfo_1;
}
pub fn seL4_MessageInfo_ptr_get_extraCaps(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t) callconv(.C) seL4_Uint64 {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 384)) >> @intCast(7)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_MessageInfo_ptr_set_extraCaps(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 384) >> @intCast(7)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x180ull >> 7) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 463), "seL4_MessageInfo_ptr_set_extraCaps");
        }
        if (!false) break;
    }
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 384)))));
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(7)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 384))));
}
pub fn seL4_MessageInfo_get_length(arg_seL4_MessageInfo_1: seL4_MessageInfo_t) callconv(.C) seL4_Uint64 {
    var seL4_MessageInfo_1 = arg_seL4_MessageInfo_1;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 127)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_MessageInfo_set_length(arg_seL4_MessageInfo_1: seL4_MessageInfo_t, arg_v64: seL4_Uint64) callconv(.C) seL4_MessageInfo_t {
    var seL4_MessageInfo_1 = arg_seL4_MessageInfo_1;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 127) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x7full >> 0 ) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 482), "seL4_MessageInfo_set_length");
        }
        if (!false) break;
    }
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 127)))));
    seL4_MessageInfo_1.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64 << @intCast(0)))) & @as(c_ulonglong, 127)))));
    return seL4_MessageInfo_1;
}
pub fn seL4_MessageInfo_ptr_get_length(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t) callconv(.C) seL4_Uint64 {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var ret: seL4_Uint64 = undefined;
    ret = @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_ulonglong, seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))]))) & @as(c_ulonglong, 127)) >> @intCast(0)))));
    if (__builtin_expect(@as(c_long, @intFromBool(!!(false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, ret))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)))), @as(c_long, @bitCast(@as(c_long, @as(c_int, 0))))) != 0) {
        ret |= @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))));
    }
    return ret;
}
pub fn seL4_MessageInfo_ptr_set_length(arg_seL4_MessageInfo_ptr: [*c]seL4_MessageInfo_t, arg_v64: seL4_Uint64) callconv(.C) void {
    var seL4_MessageInfo_ptr = arg_seL4_MessageInfo_ptr;
    var v64 = arg_v64;
    while (true) {
        if (!((((~@as(c_ulonglong, 127) >> @intCast(0)) | @as(c_ulonglong, @bitCast(@as(c_longlong, @as(c_int, 0))))) & @as(c_ulonglong, @bitCast(@as(c_ulonglong, v64)))) == @as(c_ulonglong, @bitCast(@as(c_longlong, if (false and ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, v64))) & (@as(c_ulonglong, 1) << @intCast(@as(c_int, 63)))) != 0)) @as(c_int, 0) else @as(c_int, 0)))))) {
            __assert_fail("(((~0x7full >> 0) | 0x0) & v64) == ((0 && (v64 & (1ull << (63)))) ? 0x0 : 0)", "./sel4/shared_types_gen.h", @as(c_int, 502), "seL4_MessageInfo_ptr_set_length");
        }
        if (!false) break;
    }
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] &= @as(seL4_Uint64, @bitCast(@as(c_ulong, @truncate(~@as(c_ulonglong, 127)))));
    seL4_MessageInfo_ptr.*.words[@as(c_uint, @intCast(@as(c_int, 0)))] |= (v64 << @intCast(0)) & @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 127))));
}
pub const struct_seL4_IPCBuffer_ = extern struct {
    tag: seL4_MessageInfo_t,
    msg: [120]seL4_Word,
    userData: seL4_Word,
    caps_or_badges: [3]seL4_Word,
    receiveCNode: seL4_CPtr,
    receiveIndex: seL4_CPtr,
    receiveDepth: seL4_Word,
};
pub const seL4_IPCBuffer = struct_seL4_IPCBuffer_;
pub const seL4_CapFault_IP: c_int = 0;
pub const seL4_CapFault_Addr: c_int = 1;
pub const seL4_CapFault_InRecvPhase: c_int = 2;
pub const seL4_CapFault_LookupFailureType: c_int = 3;
pub const seL4_CapFault_BitsLeft: c_int = 4;
pub const seL4_CapFault_DepthMismatch_BitsFound: c_int = 5;
pub const seL4_CapFault_GuardMismatch_GuardFound: c_int = 5;
pub const seL4_CapFault_GuardMismatch_BitsFound: c_int = 6;
pub const _enum_pad_seL4_CapFault_Msg: c_ulong = 9223372036854775807;
pub const seL4_CapFault_Msg = c_ulong;
pub const seL4_NodeId = seL4_Word;
pub const seL4_PAddr = seL4_Word;
pub const seL4_Domain = seL4_Word;
pub const seL4_CNode = seL4_CPtr;
pub const seL4_IRQHandler = seL4_CPtr;
pub const seL4_IRQControl = seL4_CPtr;
pub const seL4_TCB = seL4_CPtr;
pub const seL4_Untyped = seL4_CPtr;
pub const seL4_DomainSet = seL4_CPtr;
pub const seL4_SchedContext = seL4_CPtr;
pub const seL4_SchedControl = seL4_CPtr;
pub const seL4_Time = seL4_Uint64;
pub extern var __sel4_ipc_buffer: [*c]seL4_IPCBuffer;
pub fn seL4_SetIPCBuffer(arg_ipc_buffer: [*c]seL4_IPCBuffer) callconv(.C) void {
    var ipc_buffer = arg_ipc_buffer;
    __sel4_ipc_buffer = ipc_buffer;
    return;
}
pub fn seL4_GetIPCBuffer() callconv(.C) [*c]seL4_IPCBuffer {
    return __sel4_ipc_buffer;
}
pub fn seL4_GetMR(arg_i: c_int) callconv(.C) seL4_Word {
    var i = arg_i;
    return seL4_GetIPCBuffer().*.msg[@as(c_uint, @intCast(i))];
}
pub fn seL4_SetMR(arg_i: c_int, arg_mr: seL4_Word) callconv(.C) void {
    var i = arg_i;
    var mr = arg_mr;
    seL4_GetIPCBuffer().*.msg[@as(c_uint, @intCast(i))] = mr;
}
pub fn seL4_GetUserData() callconv(.C) seL4_Word {
    return seL4_GetIPCBuffer().*.userData;
}
pub fn seL4_SetUserData(arg_data: seL4_Word) callconv(.C) void {
    var data = arg_data;
    seL4_GetIPCBuffer().*.userData = data;
}
pub fn seL4_GetBadge(arg_i: c_int) callconv(.C) seL4_Word {
    var i = arg_i;
    return seL4_GetIPCBuffer().*.caps_or_badges[@as(c_uint, @intCast(i))];
}
pub fn seL4_GetCap(arg_i: c_int) callconv(.C) seL4_CPtr {
    var i = arg_i;
    return @as(seL4_CPtr, @bitCast(seL4_GetIPCBuffer().*.caps_or_badges[@as(c_uint, @intCast(i))]));
}
pub fn seL4_SetCap(arg_i: c_int, arg_cptr: seL4_CPtr) callconv(.C) void {
    var i = arg_i;
    var cptr = arg_cptr;
    seL4_GetIPCBuffer().*.caps_or_badges[@as(c_uint, @intCast(i))] = @as(seL4_Word, @bitCast(cptr));
}
pub fn seL4_GetCapReceivePath(arg_receiveCNode: [*c]seL4_CPtr, arg_receiveIndex: [*c]seL4_CPtr, arg_receiveDepth: [*c]seL4_Word) callconv(.C) void {
    var receiveCNode = arg_receiveCNode;
    var receiveIndex = arg_receiveIndex;
    var receiveDepth = arg_receiveDepth;
    var ipcbuffer: [*c]seL4_IPCBuffer = seL4_GetIPCBuffer();
    if (receiveCNode != @as([*c]seL4_CPtr, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        receiveCNode.* = ipcbuffer.*.receiveCNode;
    }
    if (receiveIndex != @as([*c]seL4_CPtr, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        receiveIndex.* = ipcbuffer.*.receiveIndex;
    }
    if (receiveDepth != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        receiveDepth.* = ipcbuffer.*.receiveDepth;
    }
}
pub fn seL4_SetCapReceivePath(arg_receiveCNode: seL4_CPtr, arg_receiveIndex: seL4_CPtr, arg_receiveDepth: seL4_Word) callconv(.C) void {
    var receiveCNode = arg_receiveCNode;
    var receiveIndex = arg_receiveIndex;
    var receiveDepth = arg_receiveDepth;
    var ipcbuffer: [*c]seL4_IPCBuffer = seL4_GetIPCBuffer();
    ipcbuffer.*.receiveCNode = receiveCNode;
    ipcbuffer.*.receiveIndex = receiveIndex;
    ipcbuffer.*.receiveDepth = receiveDepth;
}
pub fn seL4_Send(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t) callconv(.C) void {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    libsel4cp.zig_arm_sys_send(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysSend))), dest, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], seL4_GetMR(@as(c_int, 0)), seL4_GetMR(@as(c_int, 1)), seL4_GetMR(@as(c_int, 2)), seL4_GetMR(@as(c_int, 3)));
}
pub fn seL4_Recv(arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word, arg_reply: seL4_CPtr) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var sender = arg_sender;
    var reply = arg_reply;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = undefined;
    var msg1: seL4_Word = undefined;
    var msg2: seL4_Word = undefined;
    var msg3: seL4_Word = undefined;
    arm_sys_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysRecv))), src, &badge, &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, reply);
    seL4_SetMR(@as(c_int, 0), msg0);
    seL4_SetMR(@as(c_int, 1), msg1);
    seL4_SetMR(@as(c_int, 2), msg2);
    seL4_SetMR(@as(c_int, 3), msg3);
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_Call(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t) callconv(.C) seL4_MessageInfo_t {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    var info: seL4_MessageInfo_t = undefined;
    var msg0: seL4_Word = seL4_GetMR(@as(c_int, 0));
    var msg1: seL4_Word = seL4_GetMR(@as(c_int, 1));
    var msg2: seL4_Word = seL4_GetMR(@as(c_int, 2));
    var msg3: seL4_Word = seL4_GetMR(@as(c_int, 3));
    libsel4cp.zig_arm_sys_send_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysCall))), dest, &dest, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
    seL4_SetMR(@as(c_int, 0), msg0);
    seL4_SetMR(@as(c_int, 1), msg1);
    seL4_SetMR(@as(c_int, 2), msg2);
    seL4_SetMR(@as(c_int, 3), msg3);
    return info;
}
pub fn seL4_NBSend(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t) callconv(.C) void {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    libsel4cp.zig_arm_sys_send(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysNBSend))), dest, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], seL4_GetMR(@as(c_int, 0)), seL4_GetMR(@as(c_int, 1)), seL4_GetMR(@as(c_int, 2)), seL4_GetMR(@as(c_int, 3)));
}
pub fn seL4_ReplyRecv(arg_src: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_sender: [*c]seL4_Word, arg_reply: seL4_CPtr) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var msgInfo = arg_msgInfo;
    var sender = arg_sender;
    var reply = arg_reply;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = undefined;
    var msg1: seL4_Word = undefined;
    var msg2: seL4_Word = undefined;
    var msg3: seL4_Word = undefined;
    msg0 = seL4_GetMR(@as(c_int, 0));
    msg1 = seL4_GetMR(@as(c_int, 1));
    msg2 = seL4_GetMR(@as(c_int, 2));
    msg3 = seL4_GetMR(@as(c_int, 3));
    libsel4cp.zig_arm_sys_send_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysReplyRecv))), src, &badge, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, reply);
    seL4_SetMR(@as(c_int, 0), msg0);
    seL4_SetMR(@as(c_int, 1), msg1);
    seL4_SetMR(@as(c_int, 2), msg2);
    seL4_SetMR(@as(c_int, 3), msg3);
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_NBRecv(arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word, arg_reply: seL4_CPtr) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var sender = arg_sender;
    var reply = arg_reply;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = undefined;
    var msg1: seL4_Word = undefined;
    var msg2: seL4_Word = undefined;
    var msg3: seL4_Word = undefined;
    arm_sys_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysNBRecv))), src, &badge, &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, reply);
    seL4_SetMR(@as(c_int, 0), msg0);
    seL4_SetMR(@as(c_int, 1), msg1);
    seL4_SetMR(@as(c_int, 2), msg2);
    seL4_SetMR(@as(c_int, 3), msg3);
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_NBSendRecv(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word, arg_reply: seL4_CPtr) callconv(.C) seL4_MessageInfo_t {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    var src = arg_src;
    var sender = arg_sender;
    var reply = arg_reply;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = undefined;
    var msg1: seL4_Word = undefined;
    var msg2: seL4_Word = undefined;
    var msg3: seL4_Word = undefined;
    msg0 = seL4_GetMR(@as(c_int, 0));
    msg1 = seL4_GetMR(@as(c_int, 1));
    msg2 = seL4_GetMR(@as(c_int, 2));
    msg3 = seL4_GetMR(@as(c_int, 3));
    arm_sys_nbsend_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysNBSendRecv))), dest, src, &badge, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, reply);
    seL4_SetMR(@as(c_int, 0), msg0);
    seL4_SetMR(@as(c_int, 1), msg1);
    seL4_SetMR(@as(c_int, 2), msg2);
    seL4_SetMR(@as(c_int, 3), msg3);
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_NBSendWait(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    var src = arg_src;
    var sender = arg_sender;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = undefined;
    var msg1: seL4_Word = undefined;
    var msg2: seL4_Word = undefined;
    var msg3: seL4_Word = undefined;
    msg0 = seL4_GetMR(@as(c_int, 0));
    msg1 = seL4_GetMR(@as(c_int, 1));
    msg2 = seL4_GetMR(@as(c_int, 2));
    msg3 = seL4_GetMR(@as(c_int, 3));
    arm_sys_nbsend_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysNBSendWait))), @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), src, &badge, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, dest);
    seL4_SetMR(@as(c_int, 0), msg0);
    seL4_SetMR(@as(c_int, 1), msg1);
    seL4_SetMR(@as(c_int, 2), msg2);
    seL4_SetMR(@as(c_int, 3), msg3);
    if (sender != null) {
        sender.* = badge;
    }
    return info;
} // ./sel4/arch/syscalls.h:490:5: warning: TODO implement translation of stmt class GCCAsmStmtClass
// ./sel4/arch/syscalls.h:487:26: warning: unable to translate function, demoted to extern
pub extern fn seL4_Yield() callconv(.C) void;
pub fn seL4_Wait(arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var sender = arg_sender;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = undefined;
    var msg1: seL4_Word = undefined;
    var msg2: seL4_Word = undefined;
    var msg3: seL4_Word = undefined;
    arm_sys_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysWait))), src, &badge, &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
    seL4_SetMR(@as(c_int, 0), msg0);
    seL4_SetMR(@as(c_int, 1), msg1);
    seL4_SetMR(@as(c_int, 2), msg2);
    seL4_SetMR(@as(c_int, 3), msg3);
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_NBWait(arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var sender = arg_sender;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = undefined;
    var msg1: seL4_Word = undefined;
    var msg2: seL4_Word = undefined;
    var msg3: seL4_Word = undefined;
    arm_sys_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysNBWait))), src, &badge, &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
    seL4_SetMR(@as(c_int, 0), msg0);
    seL4_SetMR(@as(c_int, 1), msg1);
    seL4_SetMR(@as(c_int, 2), msg2);
    seL4_SetMR(@as(c_int, 3), msg3);
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_Poll(arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var sender = arg_sender;
    return seL4_NBWait(src, sender);
}
pub fn seL4_Signal(arg_dest: seL4_CPtr) callconv(.C) void {
    var dest = arg_dest;
    arm_sys_send_null(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysSend))), dest, seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0))))).words[@as(c_uint, @intCast(@as(c_int, 0)))]);
}
pub fn seL4_DebugPutChar(arg_c: u8) callconv(.C) void {
    var c = arg_c;
    var unused0: seL4_Word = 0;
    var unused1: seL4_Word = 0;
    var unused2: seL4_Word = 0;
    var unused3: seL4_Word = 0;
    var unused4: seL4_Word = 0;
    var unused5: seL4_Word = 0;
    libsel4cp.zig_arm_sys_send_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysDebugPutChar))), @as(seL4_Word, @bitCast(@as(c_ulong, c))), &unused0, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), &unused1, &unused2, &unused3, &unused4, &unused5, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
}
pub fn seL4_DebugDumpScheduler() callconv(.C) void {
    var unused0: seL4_Word = 0;
    var unused1: seL4_Word = 0;
    var unused2: seL4_Word = 0;
    var unused3: seL4_Word = 0;
    var unused4: seL4_Word = 0;
    var unused5: seL4_Word = 0;
    libsel4cp.zig_arm_sys_send_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysDebugDumpScheduler))), @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), &unused0, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), &unused1, &unused2, &unused3, &unused4, &unused5, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
}
pub fn seL4_DebugHalt() callconv(.C) void {
    arm_sys_null(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysDebugHalt))));
} // ./sel4/arch/syscalls.h:617:5: warning: TODO implement translation of stmt class GCCAsmStmtClass
// ./sel4/arch/syscalls.h:614:26: warning: unable to translate function, demoted to extern
pub extern fn seL4_DebugSnapshot() callconv(.C) void;
pub fn seL4_DebugCapIdentify(arg_cap: seL4_CPtr) callconv(.C) seL4_Uint32 {
    var cap = arg_cap;
    var unused0: seL4_Word = 0;
    var unused1: seL4_Word = 0;
    var unused2: seL4_Word = 0;
    var unused3: seL4_Word = 0;
    var unused4: seL4_Word = 0;
    libsel4cp.zig_arm_sys_send_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysDebugCapIdentify))), cap, &cap, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), &unused0, &unused1, &unused2, &unused3, &unused4, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
    return @as(seL4_Uint32, @bitCast(@as(c_uint, @truncate(cap))));
}
pub fn seL4_DebugNameThread(arg_tcb: seL4_CPtr, arg_name: [*c]const u8) callconv(.C) void {
    var tcb = arg_tcb;
    var name = arg_name;
    _ = strcpy(@as([*c]u8, @ptrCast(@alignCast(@as([*c]seL4_Word, @ptrCast(@alignCast(&seL4_GetIPCBuffer().*.msg)))))), name);
    var unused0: seL4_Word = 0;
    var unused1: seL4_Word = 0;
    var unused2: seL4_Word = 0;
    var unused3: seL4_Word = 0;
    var unused4: seL4_Word = 0;
    var unused5: seL4_Word = 0;
    libsel4cp.zig_arm_sys_send_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysDebugNameThread))), tcb, &unused0, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), &unused1, &unused2, &unused3, &unused4, &unused5, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
} // ./sel4/sel4_arch/syscalls.h:71:5: warning: TODO implement translation of stmt class GCCAsmStmtClass
// ./sel4/sel4_arch/syscalls.h:57:20: warning: unable to translate function, demoted to extern
pub extern fn arm_sys_send(arg_sys: seL4_Word, arg_dest: seL4_Word, arg_info_arg: seL4_Word, arg_mr0: seL4_Word, arg_mr1: seL4_Word, arg_mr2: seL4_Word, arg_mr3: seL4_Word) callconv(.C) void; // ./sel4/sel4_arch/syscalls.h:109:5: warning: TODO implement translation of stmt class GCCAsmStmtClass
// ./sel4/sel4_arch/syscalls.h:102:20: warning: unable to translate function, demoted to extern
pub extern fn arm_sys_send_null(arg_sys: seL4_Word, arg_src: seL4_Word, arg_info_arg: seL4_Word) callconv(.C) void; // ./sel4/sel4_arch/syscalls.h:131:5: warning: TODO implement translation of stmt class GCCAsmStmtClass
// ./sel4/sel4_arch/syscalls.h:116:20: warning: unable to translate function, demoted to extern
pub extern fn arm_sys_recv(arg_sys: seL4_Word, arg_src: seL4_Word, arg_out_badge: [*c]seL4_Word, arg_out_info: [*c]seL4_Word, arg_out_mr0: [*c]seL4_Word, arg_out_mr1: [*c]seL4_Word, arg_out_mr2: [*c]seL4_Word, arg_out_mr3: [*c]seL4_Word, arg_reply: seL4_Word) callconv(.C) void; // ./sel4/sel4_arch/syscalls.h:162:5: warning: TODO implement translation of stmt class GCCAsmStmtClass
// ./sel4/sel4_arch/syscalls.h:146:20: warning: unable to translate function, demoted to extern
pub extern fn arm_sys_send_recv(arg_sys: seL4_Word, arg_dest: seL4_Word, arg_out_badge: [*c]seL4_Word, arg_info_arg: seL4_Word, arg_out_info: [*c]seL4_Word, arg_in_out_mr0: [*c]seL4_Word, arg_in_out_mr1: [*c]seL4_Word, arg_in_out_mr2: [*c]seL4_Word, arg_in_out_mr3: [*c]seL4_Word, arg_reply: seL4_Word) callconv(.C) void; // ./sel4/sel4_arch/syscalls.h:197:5: warning: TODO implement translation of stmt class GCCAsmStmtClass
// ./sel4/sel4_arch/syscalls.h:178:20: warning: unable to translate function, demoted to extern
pub extern fn arm_sys_nbsend_recv(arg_sys: seL4_Word, arg_dest: seL4_Word, arg_src: seL4_Word, arg_out_badge: [*c]seL4_Word, arg_info_arg: seL4_Word, arg_out_info: [*c]seL4_Word, arg_in_out_mr0: [*c]seL4_Word, arg_in_out_mr1: [*c]seL4_Word, arg_in_out_mr2: [*c]seL4_Word, arg_in_out_mr3: [*c]seL4_Word, arg_reply: seL4_Word) callconv(.C) void; // ./sel4/sel4_arch/syscalls.h:217:5: warning: TODO implement translation of stmt class GCCAsmStmtClass
// ./sel4/sel4_arch/syscalls.h:214:20: warning: unable to translate function, demoted to extern
pub extern fn arm_sys_null(arg_sys: seL4_Word) callconv(.C) void;
pub fn seL4_SendWithMRs(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_mr0: [*c]seL4_Word, arg_mr1: [*c]seL4_Word, arg_mr2: [*c]seL4_Word, arg_mr3: [*c]seL4_Word) callconv(.C) void {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    var mr0 = arg_mr0;
    var mr1 = arg_mr1;
    var mr2 = arg_mr2;
    var mr3 = arg_mr3;
    libsel4cp.zig_arm_sys_send(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysSend))), dest, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], if ((mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) mr0.* else @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), if ((mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) mr1.* else @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), if ((mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) mr2.* else @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), if ((mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) mr3.* else @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
}
pub fn seL4_NBSendWithMRs(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_mr0: [*c]seL4_Word, arg_mr1: [*c]seL4_Word, arg_mr2: [*c]seL4_Word, arg_mr3: [*c]seL4_Word) callconv(.C) void {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    var mr0 = arg_mr0;
    var mr1 = arg_mr1;
    var mr2 = arg_mr2;
    var mr3 = arg_mr3;
    libsel4cp.zig_arm_sys_send(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysNBSend))), dest, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], if ((mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) mr0.* else @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), if ((mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) mr1.* else @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), if ((mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) mr2.* else @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), if ((mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) mr3.* else @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
}
pub fn seL4_RecvWithMRs(arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word, arg_reply: seL4_CPtr, arg_mr0: [*c]seL4_Word, arg_mr1: [*c]seL4_Word, arg_mr2: [*c]seL4_Word, arg_mr3: [*c]seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var sender = arg_sender;
    var reply = arg_reply;
    var mr0 = arg_mr0;
    var mr1 = arg_mr1;
    var mr2 = arg_mr2;
    var mr3 = arg_mr3;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = 0;
    var msg1: seL4_Word = 0;
    var msg2: seL4_Word = 0;
    var msg3: seL4_Word = 0;
    arm_sys_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysRecv))), src, &badge, &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, reply);
    if (mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr0.* = msg0;
    }
    if (mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr1.* = msg1;
    }
    if (mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr2.* = msg2;
    }
    if (mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr3.* = msg3;
    }
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_CallWithMRs(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_mr0: [*c]seL4_Word, arg_mr1: [*c]seL4_Word, arg_mr2: [*c]seL4_Word, arg_mr3: [*c]seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    var mr0 = arg_mr0;
    var mr1 = arg_mr1;
    var mr2 = arg_mr2;
    var mr3 = arg_mr3;
    var info: seL4_MessageInfo_t = undefined;
    var msg0: seL4_Word = 0;
    var msg1: seL4_Word = 0;
    var msg2: seL4_Word = 0;
    var msg3: seL4_Word = 0;
    if ((mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) {
        msg0 = mr0.*;
    }
    if ((mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))))) {
        msg1 = mr1.*;
    }
    if ((mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))))) {
        msg2 = mr2.*;
    }
    if ((mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))))) {
        msg3 = mr3.*;
    }
    libsel4cp.zig_arm_sys_send_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysCall))), dest, &dest, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
    if (mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr0.* = msg0;
    }
    if (mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr1.* = msg1;
    }
    if (mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr2.* = msg2;
    }
    if (mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr3.* = msg3;
    }
    return info;
}
pub fn seL4_ReplyRecvWithMRs(arg_src: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_sender: [*c]seL4_Word, arg_mr0: [*c]seL4_Word, arg_mr1: [*c]seL4_Word, arg_mr2: [*c]seL4_Word, arg_mr3: [*c]seL4_Word, arg_reply: seL4_CPtr) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var msgInfo = arg_msgInfo;
    var sender = arg_sender;
    var mr0 = arg_mr0;
    var mr1 = arg_mr1;
    var mr2 = arg_mr2;
    var mr3 = arg_mr3;
    var reply = arg_reply;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = 0;
    var msg1: seL4_Word = 0;
    var msg2: seL4_Word = 0;
    var msg3: seL4_Word = 0;
    if ((mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) {
        msg0 = mr0.*;
    }
    if ((mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))))) {
        msg1 = mr1.*;
    }
    if ((mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))))) {
        msg2 = mr2.*;
    }
    if ((mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))))) {
        msg3 = mr3.*;
    }
    libsel4cp.zig_arm_sys_send_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysReplyRecv))), src, &badge, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, reply);
    if (mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr0.* = msg0;
    }
    if (mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr1.* = msg1;
    }
    if (mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr2.* = msg2;
    }
    if (mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr3.* = msg3;
    }
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_NBSendRecvWithMRs(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word, arg_mr0: [*c]seL4_Word, arg_mr1: [*c]seL4_Word, arg_mr2: [*c]seL4_Word, arg_mr3: [*c]seL4_Word, arg_reply: seL4_CPtr) callconv(.C) seL4_MessageInfo_t {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    var src = arg_src;
    var sender = arg_sender;
    var mr0 = arg_mr0;
    var mr1 = arg_mr1;
    var mr2 = arg_mr2;
    var mr3 = arg_mr3;
    var reply = arg_reply;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = 0;
    var msg1: seL4_Word = 0;
    var msg2: seL4_Word = 0;
    var msg3: seL4_Word = 0;
    if ((mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) {
        msg0 = mr0.*;
    }
    if ((mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))))) {
        msg1 = mr1.*;
    }
    if ((mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))))) {
        msg2 = mr2.*;
    }
    if ((mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))))) {
        msg3 = mr3.*;
    }
    arm_sys_nbsend_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysNBSendRecv))), dest, src, &badge, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, reply);
    if (mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr0.* = msg0;
    }
    if (mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr1.* = msg1;
    }
    if (mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr2.* = msg2;
    }
    if (mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr3.* = msg3;
    }
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_NBSendWaitWithMRs(arg_dest: seL4_CPtr, arg_msgInfo: seL4_MessageInfo_t, arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word, arg_mr0: [*c]seL4_Word, arg_mr1: [*c]seL4_Word, arg_mr2: [*c]seL4_Word, arg_mr3: [*c]seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var dest = arg_dest;
    var msgInfo = arg_msgInfo;
    var src = arg_src;
    var sender = arg_sender;
    var mr0 = arg_mr0;
    var mr1 = arg_mr1;
    var mr2 = arg_mr2;
    var mr3 = arg_mr3;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = 0;
    var msg1: seL4_Word = 0;
    var msg2: seL4_Word = 0;
    var msg3: seL4_Word = 0;
    if ((mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))))) {
        msg0 = mr0.*;
    }
    if ((mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))))) {
        msg1 = mr1.*;
    }
    if ((mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))))) {
        msg2 = mr2.*;
    }
    if ((mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) and (seL4_MessageInfo_get_length(msgInfo) > @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))))) {
        msg3 = mr3.*;
    }
    arm_sys_nbsend_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysNBSendRecv))), @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))), src, &badge, msgInfo.words[@as(c_uint, @intCast(@as(c_int, 0)))], &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, dest);
    if (mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr0.* = msg0;
    }
    if (mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr1.* = msg1;
    }
    if (mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr2.* = msg2;
    }
    if (mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr3.* = msg3;
    }
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_WaitWithMRs(arg_src: seL4_CPtr, arg_sender: [*c]seL4_Word, arg_mr0: [*c]seL4_Word, arg_mr1: [*c]seL4_Word, arg_mr2: [*c]seL4_Word, arg_mr3: [*c]seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var src = arg_src;
    var sender = arg_sender;
    var mr0 = arg_mr0;
    var mr1 = arg_mr1;
    var mr2 = arg_mr2;
    var mr3 = arg_mr3;
    var info: seL4_MessageInfo_t = undefined;
    var badge: seL4_Word = undefined;
    var msg0: seL4_Word = 0;
    var msg1: seL4_Word = 0;
    var msg2: seL4_Word = 0;
    var msg3: seL4_Word = 0;
    arm_sys_recv(@as(seL4_Word, @bitCast(@as(c_long, seL4_SysWait))), src, &badge, &info.words[@as(c_uint, @intCast(@as(c_int, 0)))], &msg0, &msg1, &msg2, &msg3, @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 0)))));
    if (mr0 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr0.* = msg0;
    }
    if (mr1 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr1.* = msg1;
    }
    if (mr2 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr2.* = msg2;
    }
    if (mr3 != @as([*c]seL4_Word, @ptrCast(@alignCast(@as(?*anyopaque, @ptrFromInt(@as(c_int, 0))))))) {
        mr3.* = msg3;
    }
    if (sender != null) {
        sender.* = badge;
    }
    return info;
}
pub fn seL4_DebugPutString(arg_str: [*c]u8) callconv(.C) void {
    var str = arg_str;
    {
        var s: [*c]u8 = str;
        while (s.* != 0) : (s += 1) {
            seL4_DebugPutChar(s.*);
        }
    }
    return;
}
pub extern fn strcpy([*c]u8, [*c]const u8) [*c]u8;
pub const InvalidInvocation: c_int = 0;
pub const UntypedRetype: c_int = 1;
pub const TCBReadRegisters: c_int = 2;
pub const TCBWriteRegisters: c_int = 3;
pub const TCBCopyRegisters: c_int = 4;
pub const TCBConfigure: c_int = 5;
pub const TCBSetPriority: c_int = 6;
pub const TCBSetMCPriority: c_int = 7;
pub const TCBSetSchedParams: c_int = 8;
pub const TCBSetTimeoutEndpoint: c_int = 9;
pub const TCBSetIPCBuffer: c_int = 10;
pub const TCBSetSpace: c_int = 11;
pub const TCBSuspend: c_int = 12;
pub const TCBResume: c_int = 13;
pub const TCBBindNotification: c_int = 14;
pub const TCBUnbindNotification: c_int = 15;
pub const TCBSetTLSBase: c_int = 16;
pub const CNodeRevoke: c_int = 17;
pub const CNodeDelete: c_int = 18;
pub const CNodeCancelBadgedSends: c_int = 19;
pub const CNodeCopy: c_int = 20;
pub const CNodeMint: c_int = 21;
pub const CNodeMove: c_int = 22;
pub const CNodeMutate: c_int = 23;
pub const CNodeRotate: c_int = 24;
pub const IRQIssueIRQHandler: c_int = 25;
pub const IRQAckIRQ: c_int = 26;
pub const IRQSetIRQHandler: c_int = 27;
pub const IRQClearIRQHandler: c_int = 28;
pub const DomainSetSet: c_int = 29;
pub const SchedControlConfigureFlags: c_int = 30;
pub const SchedContextBind: c_int = 31;
pub const SchedContextUnbind: c_int = 32;
pub const SchedContextUnbindObject: c_int = 33;
pub const SchedContextConsumed: c_int = 34;
pub const SchedContextYieldTo: c_int = 35;
pub const nInvocationLabels: c_int = 36;
pub const enum_invocation_label = c_uint;
pub const ARMVSpaceClean_Data: c_int = 36;
pub const ARMVSpaceInvalidate_Data: c_int = 37;
pub const ARMVSpaceCleanInvalidate_Data: c_int = 38;
pub const ARMVSpaceUnify_Instruction: c_int = 39;
pub const ARMPageDirectoryMap: c_int = 40;
pub const ARMPageDirectoryUnmap: c_int = 41;
pub const nSeL4ArchInvocationLabels: c_int = 42;
pub const enum_sel4_arch_invocation_label = c_uint;
pub const ARMPageTableMap: c_int = 42;
pub const ARMPageTableUnmap: c_int = 43;
pub const ARMPageMap: c_int = 44;
pub const ARMPageUnmap: c_int = 45;
pub const ARMPageClean_Data: c_int = 46;
pub const ARMPageInvalidate_Data: c_int = 47;
pub const ARMPageCleanInvalidate_Data: c_int = 48;
pub const ARMPageUnify_Instruction: c_int = 49;
pub const ARMPageGetAddress: c_int = 50;
pub const ARMASIDControlMakePool: c_int = 51;
pub const ARMASIDPoolAssign: c_int = 52;
pub const ARMVCPUSetTCB: c_int = 53;
pub const ARMVCPUInjectIRQ: c_int = 54;
pub const ARMVCPUReadReg: c_int = 55;
pub const ARMVCPUWriteReg: c_int = 56;
pub const ARMVCPUAckVPPI: c_int = 57;
pub const ARMIRQIssueIRQHandlerTrigger: c_int = 58;
pub const nArchInvocationLabels: c_int = 59;
pub const enum_arch_invocation_label = c_uint;
pub const __type_int_size_incorrect = [1]c_ulong;
pub const __type_long_size_incorrect = [1]c_ulong;
pub const __type_seL4_Uint8_size_incorrect = [1]c_ulong;
pub const __type_seL4_Uint16_size_incorrect = [1]c_ulong;
pub const __type_seL4_Uint32_size_incorrect = [1]c_ulong;
pub const __type_seL4_Uint64_size_incorrect = [1]c_ulong;
pub const __type_seL4_Time_size_incorrect = [1]c_ulong;
pub const __type_seL4_Word_size_incorrect = [1]c_ulong;
pub const __type_seL4_Bool_size_incorrect = [1]c_ulong;
pub const __type_seL4_CapRights_t_size_incorrect = [1]c_ulong;
pub const __type_seL4_CPtr_size_incorrect = [1]c_ulong;
pub const __type_seL4_CNode_size_incorrect = [1]c_ulong;
pub const __type_seL4_IRQHandler_size_incorrect = [1]c_ulong;
pub const __type_seL4_IRQControl_size_incorrect = [1]c_ulong;
pub const __type_seL4_TCB_size_incorrect = [1]c_ulong;
pub const __type_seL4_Untyped_size_incorrect = [1]c_ulong;
pub const __type_seL4_DomainSet_size_incorrect = [1]c_ulong;
pub const __type_seL4_SchedContext_size_incorrect = [1]c_ulong;
pub const __type_seL4_SchedControl_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_VMAttributes_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_Page_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_PageTable_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_PageDirectory_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_PageUpperDirectory_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_PageGlobalDirectory_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_VSpace_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_ASIDControl_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_ASIDPool_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_VCPU_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_IOSpace_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_IOPageTable_size_incorrect = [1]c_ulong;
pub const __type_seL4_UserContext_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_SIDControl_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_SID_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_CBControl_size_incorrect = [1]c_ulong;
pub const __type_seL4_ARM_CB_size_incorrect = [1]c_ulong;
pub const struct_seL4_ARM_Page_GetAddress = extern struct {
    @"error": c_int,
    paddr: seL4_Word,
};
pub const seL4_ARM_Page_GetAddress_t = struct_seL4_ARM_Page_GetAddress;
pub const struct_seL4_ARM_VCPU_ReadRegs = extern struct {
    @"error": c_int,
    value: seL4_Word,
};
pub const seL4_ARM_VCPU_ReadRegs_t = struct_seL4_ARM_VCPU_ReadRegs;
pub const struct_seL4_ARM_SIDControl_GetFault = extern struct {
    @"error": c_int,
    status: seL4_Word,
    syndrome_0: seL4_Word,
    syndrome_1: seL4_Word,
};
pub const seL4_ARM_SIDControl_GetFault_t = struct_seL4_ARM_SIDControl_GetFault;
pub const struct_seL4_ARM_CB_CBGetFault = extern struct {
    @"error": c_int,
    status: seL4_Word,
    address: seL4_Word,
};
pub const seL4_ARM_CB_CBGetFault_t = struct_seL4_ARM_CB_CBGetFault;
pub const struct_seL4_TCB_GetBreakpoint = extern struct {
    @"error": c_int,
    vaddr: seL4_Word,
    type: seL4_Word,
    size: seL4_Word,
    rw: seL4_Word,
    is_enabled: seL4_Bool,
};
pub const seL4_TCB_GetBreakpoint_t = struct_seL4_TCB_GetBreakpoint;
pub const struct_seL4_TCB_ConfigureSingleStepping = extern struct {
    @"error": c_int,
    bp_was_consumed: seL4_Bool,
};
pub const seL4_TCB_ConfigureSingleStepping_t = struct_seL4_TCB_ConfigureSingleStepping;
pub const struct_seL4_SchedContext_Consumed = extern struct {
    @"error": c_int,
    consumed: seL4_Time,
};
pub const seL4_SchedContext_Consumed_t = struct_seL4_SchedContext_Consumed;
pub const struct_seL4_SchedContext_YieldTo = extern struct {
    @"error": c_int,
    consumed: seL4_Time,
};
pub const seL4_SchedContext_YieldTo_t = struct_seL4_SchedContext_YieldTo;
pub fn seL4_ARM_VSpace_Clean_Data(arg__service: seL4_ARM_VSpace, arg_start: seL4_Word, arg_end: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var start = arg_start;
    var end = arg_end;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVSpaceClean_Data))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = start;
    mr1 = end;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_VSpace_Invalidate_Data(arg__service: seL4_ARM_VSpace, arg_start: seL4_Word, arg_end: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var start = arg_start;
    var end = arg_end;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVSpaceInvalidate_Data))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = start;
    mr1 = end;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_VSpace_CleanInvalidate_Data(arg__service: seL4_ARM_VSpace, arg_start: seL4_Word, arg_end: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var start = arg_start;
    var end = arg_end;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVSpaceCleanInvalidate_Data))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = start;
    mr1 = end;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_VSpace_Unify_Instruction(arg__service: seL4_ARM_VSpace, arg_start: seL4_Word, arg_end: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var start = arg_start;
    var end = arg_end;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVSpaceUnify_Instruction))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = start;
    mr1 = end;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_PageDirectory_Map(arg__service: seL4_ARM_PageDirectory, arg_vspace: seL4_CPtr, arg_vaddr: seL4_Word, arg_attr: seL4_ARM_VMAttributes) callconv(.C) seL4_Error {
    var _service = arg__service;
    var vspace = arg_vspace;
    var vaddr = arg_vaddr;
    var attr = arg_attr;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageDirectoryMap))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), vspace);
    mr0 = vaddr;
    mr1 = @as(seL4_Word, @bitCast(attr));
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_PageDirectory_Unmap(arg__service: seL4_ARM_PageDirectory) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageDirectoryUnmap))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_PageTable_Map(arg__service: seL4_ARM_PageTable, arg_vspace: seL4_CPtr, arg_vaddr: seL4_Word, arg_attr: seL4_ARM_VMAttributes) callconv(.C) seL4_Error {
    var _service = arg__service;
    var vspace = arg_vspace;
    var vaddr = arg_vaddr;
    var attr = arg_attr;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageTableMap))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), vspace);
    mr0 = vaddr;
    mr1 = @as(seL4_Word, @bitCast(attr));
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_PageTable_Unmap(arg__service: seL4_ARM_PageTable) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageTableUnmap))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_Page_Map(arg__service: seL4_ARM_Page, arg_vspace: seL4_CPtr, arg_vaddr: seL4_Word, arg_rights: seL4_CapRights_t, arg_attr: seL4_ARM_VMAttributes) callconv(.C) seL4_Error {
    var _service = arg__service;
    var vspace = arg_vspace;
    var vaddr = arg_vaddr;
    var rights = arg_rights;
    var attr = arg_attr;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageMap))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), vspace);
    mr0 = vaddr;
    mr1 = rights.words[@as(c_uint, @intCast(@as(c_int, 0)))];
    mr2 = @as(seL4_Word, @bitCast(attr));
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_Page_Unmap(arg__service: seL4_ARM_Page) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageUnmap))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_Page_Clean_Data(arg__service: seL4_ARM_Page, arg_start_offset: seL4_Word, arg_end_offset: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var start_offset = arg_start_offset;
    var end_offset = arg_end_offset;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageClean_Data))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = start_offset;
    mr1 = end_offset;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_Page_Invalidate_Data(arg__service: seL4_ARM_Page, arg_start_offset: seL4_Word, arg_end_offset: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var start_offset = arg_start_offset;
    var end_offset = arg_end_offset;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageInvalidate_Data))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = start_offset;
    mr1 = end_offset;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_Page_CleanInvalidate_Data(arg__service: seL4_ARM_Page, arg_start_offset: seL4_Word, arg_end_offset: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var start_offset = arg_start_offset;
    var end_offset = arg_end_offset;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageCleanInvalidate_Data))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = start_offset;
    mr1 = end_offset;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_Page_Unify_Instruction(arg__service: seL4_ARM_Page, arg_start_offset: seL4_Word, arg_end_offset: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var start_offset = arg_start_offset;
    var end_offset = arg_end_offset;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageUnify_Instruction))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = start_offset;
    mr1 = end_offset;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_Page_GetAddress(arg__service: seL4_ARM_Page) callconv(.C) seL4_ARM_Page_GetAddress_t {
    var _service = arg__service;
    var result: seL4_ARM_Page_GetAddress_t = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMPageGetAddress))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result.@"error" = @as(c_int, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result.@"error" != seL4_NoError) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
        return result;
    }
    result.paddr = mr0;
    return result;
}
pub fn seL4_ARM_ASIDControl_MakePool(arg__service: seL4_ARM_ASIDControl, arg_untyped: seL4_Untyped, arg_root: seL4_CNode, arg_index: seL4_Word, arg_depth: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var untyped = arg_untyped;
    var root = arg_root;
    var index = arg_index;
    var depth = arg_depth;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMASIDControlMakePool))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), untyped);
    seL4_SetCap(@as(c_int, 1), root);
    mr0 = index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, depth))) & @as(c_ulonglong, 255)))));
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_ASIDPool_Assign(arg__service: seL4_ARM_ASIDPool, arg_vspace: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var vspace = arg_vspace;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMASIDPoolAssign))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), vspace);
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_VCPU_SetTCB(arg__service: seL4_ARM_VCPU, arg_tcb: seL4_TCB) callconv(.C) seL4_Error {
    var _service = arg__service;
    var tcb = arg_tcb;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVCPUSetTCB))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), tcb);
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_VCPU_InjectIRQ(arg__service: seL4_ARM_VCPU, arg_virq: seL4_Uint16, arg_priority: seL4_Uint8, arg_group: seL4_Uint8, arg_index: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var virq = arg_virq;
    var priority = arg_priority;
    var group = arg_group;
    var index = arg_index;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVCPUInjectIRQ))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate((((@as(c_ulonglong, @bitCast(@as(c_ulonglong, virq))) & @as(c_ulonglong, 65535)) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, priority))) & @as(c_ulonglong, 255)) << @intCast(16))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, group))) & @as(c_ulonglong, 255)) << @intCast(24))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, index))) & @as(c_ulonglong, 255)) << @intCast(32))))));
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_VCPU_ReadRegs(arg__service: seL4_ARM_VCPU, arg_field: seL4_Word) callconv(.C) seL4_ARM_VCPU_ReadRegs_t {
    var _service = arg__service;
    var field = arg_field;
    var result: seL4_ARM_VCPU_ReadRegs_t = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVCPUReadReg))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = field;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result.@"error" = @as(c_int, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result.@"error" != seL4_NoError) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
        return result;
    }
    result.value = mr0;
    return result;
}
pub fn seL4_ARM_VCPU_WriteRegs(arg__service: seL4_ARM_VCPU, arg_field: seL4_Word, arg_value: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var field = arg_field;
    var value = arg_value;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVCPUWriteReg))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = field;
    mr1 = value;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_ARM_VCPU_AckVPPI(arg__service: seL4_ARM_VCPU, arg_irq: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var irq = arg_irq;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMVCPUAckVPPI))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = irq;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_IRQControl_GetTrigger(arg__service: seL4_IRQControl, arg_irq: seL4_Word, arg_trigger: seL4_Word, arg_root: seL4_CNode, arg_index: seL4_Word, arg_depth: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var irq = arg_irq;
    var trigger = arg_trigger;
    var root = arg_root;
    var index = arg_index;
    var depth = arg_depth;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, ARMIRQIssueIRQHandlerTrigger))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 4)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), root);
    mr0 = irq;
    mr1 = trigger;
    mr2 = index;
    mr3 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, depth))) & @as(c_ulonglong, 255)))));
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_Untyped_Retype(arg__service: seL4_Untyped, arg_type: seL4_Word, arg_size_bits: seL4_Word, arg_root: seL4_CNode, arg_node_index: seL4_Word, arg_node_depth: seL4_Word, arg_node_offset: seL4_Word, arg_num_objects: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var @"type" = arg_type;
    var size_bits = arg_size_bits;
    var root = arg_root;
    var node_index = arg_node_index;
    var node_depth = arg_node_depth;
    var node_offset = arg_node_offset;
    var num_objects = arg_num_objects;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, UntypedRetype))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 6)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), root);
    mr0 = @"type";
    mr1 = size_bits;
    mr2 = node_index;
    mr3 = node_depth;
    seL4_SetMR(@as(c_int, 4), node_offset);
    seL4_SetMR(@as(c_int, 5), num_objects);
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_ReadRegisters(arg__service: seL4_TCB, arg_suspend_source: seL4_Bool, arg_arch_flags: seL4_Uint8, arg_count: seL4_Word, arg_regs: [*c]seL4_UserContext) callconv(.C) seL4_Error {
    var _service = arg__service;
    var suspend_source = arg_suspend_source;
    var arch_flags = arg_arch_flags;
    var count = arg_count;
    var regs = arg_regs;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBReadRegisters))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_longlong, suspend_source))) & @as(c_ulonglong, 1)) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, arch_flags))) & @as(c_ulonglong, 255)) << @intCast(8))))));
    mr1 = count;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    regs.*.pc = mr0;
    regs.*.sp = mr1;
    regs.*.spsr = mr2;
    regs.*.x0 = mr3;
    regs.*.x1 = seL4_GetMR(@as(c_int, 4));
    regs.*.x2 = seL4_GetMR(@as(c_int, 5));
    regs.*.x3 = seL4_GetMR(@as(c_int, 6));
    regs.*.x4 = seL4_GetMR(@as(c_int, 7));
    regs.*.x5 = seL4_GetMR(@as(c_int, 8));
    regs.*.x6 = seL4_GetMR(@as(c_int, 9));
    regs.*.x7 = seL4_GetMR(@as(c_int, 10));
    regs.*.x8 = seL4_GetMR(@as(c_int, 11));
    regs.*.x16 = seL4_GetMR(@as(c_int, 12));
    regs.*.x17 = seL4_GetMR(@as(c_int, 13));
    regs.*.x18 = seL4_GetMR(@as(c_int, 14));
    regs.*.x29 = seL4_GetMR(@as(c_int, 15));
    regs.*.x30 = seL4_GetMR(@as(c_int, 16));
    regs.*.x9 = seL4_GetMR(@as(c_int, 17));
    regs.*.x10 = seL4_GetMR(@as(c_int, 18));
    regs.*.x11 = seL4_GetMR(@as(c_int, 19));
    regs.*.x12 = seL4_GetMR(@as(c_int, 20));
    regs.*.x13 = seL4_GetMR(@as(c_int, 21));
    regs.*.x14 = seL4_GetMR(@as(c_int, 22));
    regs.*.x15 = seL4_GetMR(@as(c_int, 23));
    regs.*.x19 = seL4_GetMR(@as(c_int, 24));
    regs.*.x20 = seL4_GetMR(@as(c_int, 25));
    regs.*.x21 = seL4_GetMR(@as(c_int, 26));
    regs.*.x22 = seL4_GetMR(@as(c_int, 27));
    regs.*.x23 = seL4_GetMR(@as(c_int, 28));
    regs.*.x24 = seL4_GetMR(@as(c_int, 29));
    regs.*.x25 = seL4_GetMR(@as(c_int, 30));
    regs.*.x26 = seL4_GetMR(@as(c_int, 31));
    regs.*.x27 = seL4_GetMR(@as(c_int, 32));
    regs.*.x28 = seL4_GetMR(@as(c_int, 33));
    regs.*.tpidr_el0 = seL4_GetMR(@as(c_int, 34));
    regs.*.tpidrro_el0 = seL4_GetMR(@as(c_int, 35));
    return result;
}
pub fn seL4_TCB_WriteRegisters(arg__service: seL4_TCB, arg_resume_target: seL4_Bool, arg_arch_flags: seL4_Uint8, arg_count: seL4_Word, arg_regs: [*c]seL4_UserContext) callconv(.C) seL4_Error {
    var _service = arg__service;
    var resume_target = arg_resume_target;
    var arch_flags = arg_arch_flags;
    var count = arg_count;
    var regs = arg_regs;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBWriteRegisters))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 38)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate((@as(c_ulonglong, @bitCast(@as(c_longlong, resume_target))) & @as(c_ulonglong, 1)) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, arch_flags))) & @as(c_ulonglong, 255)) << @intCast(8))))));
    mr1 = count;
    mr2 = regs.*.pc;
    mr3 = regs.*.sp;
    seL4_SetMR(@as(c_int, 4), regs.*.spsr);
    seL4_SetMR(@as(c_int, 5), regs.*.x0);
    seL4_SetMR(@as(c_int, 6), regs.*.x1);
    seL4_SetMR(@as(c_int, 7), regs.*.x2);
    seL4_SetMR(@as(c_int, 8), regs.*.x3);
    seL4_SetMR(@as(c_int, 9), regs.*.x4);
    seL4_SetMR(@as(c_int, 10), regs.*.x5);
    seL4_SetMR(@as(c_int, 11), regs.*.x6);
    seL4_SetMR(@as(c_int, 12), regs.*.x7);
    seL4_SetMR(@as(c_int, 13), regs.*.x8);
    seL4_SetMR(@as(c_int, 14), regs.*.x16);
    seL4_SetMR(@as(c_int, 15), regs.*.x17);
    seL4_SetMR(@as(c_int, 16), regs.*.x18);
    seL4_SetMR(@as(c_int, 17), regs.*.x29);
    seL4_SetMR(@as(c_int, 18), regs.*.x30);
    seL4_SetMR(@as(c_int, 19), regs.*.x9);
    seL4_SetMR(@as(c_int, 20), regs.*.x10);
    seL4_SetMR(@as(c_int, 21), regs.*.x11);
    seL4_SetMR(@as(c_int, 22), regs.*.x12);
    seL4_SetMR(@as(c_int, 23), regs.*.x13);
    seL4_SetMR(@as(c_int, 24), regs.*.x14);
    seL4_SetMR(@as(c_int, 25), regs.*.x15);
    seL4_SetMR(@as(c_int, 26), regs.*.x19);
    seL4_SetMR(@as(c_int, 27), regs.*.x20);
    seL4_SetMR(@as(c_int, 28), regs.*.x21);
    seL4_SetMR(@as(c_int, 29), regs.*.x22);
    seL4_SetMR(@as(c_int, 30), regs.*.x23);
    seL4_SetMR(@as(c_int, 31), regs.*.x24);
    seL4_SetMR(@as(c_int, 32), regs.*.x25);
    seL4_SetMR(@as(c_int, 33), regs.*.x26);
    seL4_SetMR(@as(c_int, 34), regs.*.x27);
    seL4_SetMR(@as(c_int, 35), regs.*.x28);
    seL4_SetMR(@as(c_int, 36), regs.*.tpidr_el0);
    seL4_SetMR(@as(c_int, 37), regs.*.tpidrro_el0);
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_CopyRegisters(arg__service: seL4_TCB, arg_source: seL4_TCB, arg_suspend_source: seL4_Bool, arg_resume_target: seL4_Bool, arg_transfer_frame: seL4_Bool, arg_transfer_integer: seL4_Bool, arg_arch_flags: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var source = arg_source;
    var suspend_source = arg_suspend_source;
    var resume_target = arg_resume_target;
    var transfer_frame = arg_transfer_frame;
    var transfer_integer = arg_transfer_integer;
    var arch_flags = arg_arch_flags;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBCopyRegisters))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), source);
    mr0 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(((((@as(c_ulonglong, @bitCast(@as(c_longlong, suspend_source))) & @as(c_ulonglong, 1)) | ((@as(c_ulonglong, @bitCast(@as(c_longlong, resume_target))) & @as(c_ulonglong, 1)) << @intCast(1))) | ((@as(c_ulonglong, @bitCast(@as(c_longlong, transfer_frame))) & @as(c_ulonglong, 1)) << @intCast(2))) | ((@as(c_ulonglong, @bitCast(@as(c_longlong, transfer_integer))) & @as(c_ulonglong, 1)) << @intCast(3))) | ((@as(c_ulonglong, @bitCast(@as(c_ulonglong, arch_flags))) & @as(c_ulonglong, 255)) << @intCast(8))))));
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_Configure(arg__service: seL4_TCB, arg_cspace_root: seL4_CNode, arg_cspace_root_data: seL4_Word, arg_vspace_root: seL4_CPtr, arg_vspace_root_data: seL4_Word, arg_buffer: seL4_Word, arg_bufferFrame: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var cspace_root = arg_cspace_root;
    var cspace_root_data = arg_cspace_root_data;
    var vspace_root = arg_vspace_root;
    var vspace_root_data = arg_vspace_root_data;
    var buffer = arg_buffer;
    var bufferFrame = arg_bufferFrame;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBConfigure))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), cspace_root);
    seL4_SetCap(@as(c_int, 1), vspace_root);
    seL4_SetCap(@as(c_int, 2), bufferFrame);
    mr0 = cspace_root_data;
    mr1 = vspace_root_data;
    mr2 = buffer;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_SetPriority(arg__service: seL4_TCB, arg_authority: seL4_TCB, arg_priority: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var authority = arg_authority;
    var priority = arg_priority;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBSetPriority))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), authority);
    mr0 = priority;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_SetMCPriority(arg__service: seL4_TCB, arg_authority: seL4_TCB, arg_mcp: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var authority = arg_authority;
    var mcp = arg_mcp;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBSetMCPriority))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), authority);
    mr0 = mcp;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_SetSchedParams(arg__service: seL4_TCB, arg_authority: seL4_TCB, arg_mcp: seL4_Word, arg_priority: seL4_Word, arg_sched_context: seL4_CPtr, arg_fault_ep: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var authority = arg_authority;
    var mcp = arg_mcp;
    var priority = arg_priority;
    var sched_context = arg_sched_context;
    var fault_ep = arg_fault_ep;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBSetSchedParams))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), authority);
    seL4_SetCap(@as(c_int, 1), sched_context);
    seL4_SetCap(@as(c_int, 2), fault_ep);
    mr0 = mcp;
    mr1 = priority;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_SetTimeoutEndpoint(arg__service: seL4_TCB, arg_timeout_fault_ep: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var timeout_fault_ep = arg_timeout_fault_ep;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBSetTimeoutEndpoint))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), timeout_fault_ep);
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_SetIPCBuffer(arg__service: seL4_TCB, arg_buffer: seL4_Word, arg_bufferFrame: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var buffer = arg_buffer;
    var bufferFrame = arg_bufferFrame;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBSetIPCBuffer))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), bufferFrame);
    mr0 = buffer;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_SetSpace(arg__service: seL4_TCB, arg_fault_ep: seL4_CPtr, arg_cspace_root: seL4_CNode, arg_cspace_root_data: seL4_Word, arg_vspace_root: seL4_CPtr, arg_vspace_root_data: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var fault_ep = arg_fault_ep;
    var cspace_root = arg_cspace_root;
    var cspace_root_data = arg_cspace_root_data;
    var vspace_root = arg_vspace_root;
    var vspace_root_data = arg_vspace_root_data;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBSetSpace))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), fault_ep);
    seL4_SetCap(@as(c_int, 1), cspace_root);
    seL4_SetCap(@as(c_int, 2), vspace_root);
    mr0 = cspace_root_data;
    mr1 = vspace_root_data;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_Suspend(arg__service: seL4_TCB) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBSuspend))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_Resume(arg__service: seL4_TCB) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBResume))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_BindNotification(arg__service: seL4_TCB, arg_notification: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var notification = arg_notification;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBBindNotification))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), notification);
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_UnbindNotification(arg__service: seL4_TCB) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBUnbindNotification))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_TCB_SetTLSBase(arg__service: seL4_TCB, arg_tls_base: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var tls_base = arg_tls_base;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, TCBSetTLSBase))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = tls_base;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_CNode_Revoke(arg__service: seL4_CNode, arg_index: seL4_Word, arg_depth: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var index = arg_index;
    var depth = arg_depth;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, CNodeRevoke))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, depth))) & @as(c_ulonglong, 255)))));
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_CNode_Delete(arg__service: seL4_CNode, arg_index: seL4_Word, arg_depth: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var index = arg_index;
    var depth = arg_depth;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, CNodeDelete))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, depth))) & @as(c_ulonglong, 255)))));
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_CNode_CancelBadgedSends(arg__service: seL4_CNode, arg_index: seL4_Word, arg_depth: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var index = arg_index;
    var depth = arg_depth;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, CNodeCancelBadgedSends))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, depth))) & @as(c_ulonglong, 255)))));
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_CNode_Copy(arg__service: seL4_CNode, arg_dest_index: seL4_Word, arg_dest_depth: seL4_Uint8, arg_src_root: seL4_CNode, arg_src_index: seL4_Word, arg_src_depth: seL4_Uint8, arg_rights: seL4_CapRights_t) callconv(.C) seL4_Error {
    var _service = arg__service;
    var dest_index = arg_dest_index;
    var dest_depth = arg_dest_depth;
    var src_root = arg_src_root;
    var src_index = arg_src_index;
    var src_depth = arg_src_depth;
    var rights = arg_rights;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, CNodeCopy))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 5)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), src_root);
    mr0 = dest_index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, dest_depth))) & @as(c_ulonglong, 255)))));
    mr2 = src_index;
    mr3 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, src_depth))) & @as(c_ulonglong, 255)))));
    seL4_SetMR(@as(c_int, 4), rights.words[@as(c_uint, @intCast(@as(c_int, 0)))]);
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_CNode_Mint(arg__service: seL4_CNode, arg_dest_index: seL4_Word, arg_dest_depth: seL4_Uint8, arg_src_root: seL4_CNode, arg_src_index: seL4_Word, arg_src_depth: seL4_Uint8, arg_rights: seL4_CapRights_t, arg_badge: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var dest_index = arg_dest_index;
    var dest_depth = arg_dest_depth;
    var src_root = arg_src_root;
    var src_index = arg_src_index;
    var src_depth = arg_src_depth;
    var rights = arg_rights;
    var badge = arg_badge;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, CNodeMint))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 6)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), src_root);
    mr0 = dest_index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, dest_depth))) & @as(c_ulonglong, 255)))));
    mr2 = src_index;
    mr3 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, src_depth))) & @as(c_ulonglong, 255)))));
    seL4_SetMR(@as(c_int, 4), rights.words[@as(c_uint, @intCast(@as(c_int, 0)))]);
    seL4_SetMR(@as(c_int, 5), badge);
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_CNode_Move(arg__service: seL4_CNode, arg_dest_index: seL4_Word, arg_dest_depth: seL4_Uint8, arg_src_root: seL4_CNode, arg_src_index: seL4_Word, arg_src_depth: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var dest_index = arg_dest_index;
    var dest_depth = arg_dest_depth;
    var src_root = arg_src_root;
    var src_index = arg_src_index;
    var src_depth = arg_src_depth;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, CNodeMove))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 4)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), src_root);
    mr0 = dest_index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, dest_depth))) & @as(c_ulonglong, 255)))));
    mr2 = src_index;
    mr3 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, src_depth))) & @as(c_ulonglong, 255)))));
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_CNode_Mutate(arg__service: seL4_CNode, arg_dest_index: seL4_Word, arg_dest_depth: seL4_Uint8, arg_src_root: seL4_CNode, arg_src_index: seL4_Word, arg_src_depth: seL4_Uint8, arg_badge: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var dest_index = arg_dest_index;
    var dest_depth = arg_dest_depth;
    var src_root = arg_src_root;
    var src_index = arg_src_index;
    var src_depth = arg_src_depth;
    var badge = arg_badge;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, CNodeMutate))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 5)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), src_root);
    mr0 = dest_index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, dest_depth))) & @as(c_ulonglong, 255)))));
    mr2 = src_index;
    mr3 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, src_depth))) & @as(c_ulonglong, 255)))));
    seL4_SetMR(@as(c_int, 4), badge);
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_CNode_Rotate(arg__service: seL4_CNode, arg_dest_index: seL4_Word, arg_dest_depth: seL4_Uint8, arg_dest_badge: seL4_Word, arg_pivot_root: seL4_CNode, arg_pivot_index: seL4_Word, arg_pivot_depth: seL4_Uint8, arg_pivot_badge: seL4_Word, arg_src_root: seL4_CNode, arg_src_index: seL4_Word, arg_src_depth: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var dest_index = arg_dest_index;
    var dest_depth = arg_dest_depth;
    var dest_badge = arg_dest_badge;
    var pivot_root = arg_pivot_root;
    var pivot_index = arg_pivot_index;
    var pivot_depth = arg_pivot_depth;
    var pivot_badge = arg_pivot_badge;
    var src_root = arg_src_root;
    var src_index = arg_src_index;
    var src_depth = arg_src_depth;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, CNodeRotate))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 8)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), pivot_root);
    seL4_SetCap(@as(c_int, 1), src_root);
    mr0 = dest_index;
    mr1 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, dest_depth))) & @as(c_ulonglong, 255)))));
    mr2 = dest_badge;
    mr3 = pivot_index;
    seL4_SetMR(@as(c_int, 4), @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, pivot_depth))) & @as(c_ulonglong, 255))))));
    seL4_SetMR(@as(c_int, 5), pivot_badge);
    seL4_SetMR(@as(c_int, 6), src_index);
    seL4_SetMR(@as(c_int, 7), @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, src_depth))) & @as(c_ulonglong, 255))))));
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_IRQControl_Get(arg__service: seL4_IRQControl, arg_irq: seL4_Word, arg_root: seL4_CNode, arg_index: seL4_Word, arg_depth: seL4_Uint8) callconv(.C) seL4_Error {
    var _service = arg__service;
    var irq = arg_irq;
    var root = arg_root;
    var index = arg_index;
    var depth = arg_depth;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, IRQIssueIRQHandler))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), root);
    mr0 = irq;
    mr1 = index;
    mr2 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, depth))) & @as(c_ulonglong, 255)))));
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_IRQHandler_Ack(arg__service: seL4_IRQHandler) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, IRQAckIRQ))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_IRQHandler_SetNotification(arg__service: seL4_IRQHandler, arg_notification: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var notification = arg_notification;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, IRQSetIRQHandler))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), notification);
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_IRQHandler_Clear(arg__service: seL4_IRQHandler) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, IRQClearIRQHandler))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_DomainSet_Set(arg__service: seL4_DomainSet, arg_domain: seL4_Uint8, arg_thread: seL4_TCB) callconv(.C) seL4_Error {
    var _service = arg__service;
    var domain = arg_domain;
    var thread = arg_thread;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, DomainSetSet))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), thread);
    mr0 = @as(seL4_Word, @bitCast(@as(c_ulong, @truncate(@as(c_ulonglong, @bitCast(@as(c_ulonglong, domain))) & @as(c_ulonglong, 255)))));
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_SchedControl_ConfigureFlags(arg__service: seL4_SchedControl, arg_schedcontext: seL4_SchedContext, arg_budget: seL4_Time, arg_period: seL4_Time, arg_extra_refills: seL4_Word, arg_badge: seL4_Word, arg_flags: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var schedcontext = arg_schedcontext;
    var budget = arg_budget;
    var period = arg_period;
    var extra_refills = arg_extra_refills;
    var badge = arg_badge;
    var flags = arg_flags;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, SchedControlConfigureFlags))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 5)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), schedcontext);
    mr0 = budget;
    mr1 = period;
    mr2 = extra_refills;
    mr3 = badge;
    seL4_SetMR(@as(c_int, 4), flags);
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_SchedContext_Bind(arg__service: seL4_SchedContext, arg_cap: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var cap = arg_cap;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, SchedContextBind))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), cap);
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_SchedContext_Unbind(arg__service: seL4_SchedContext) callconv(.C) seL4_Error {
    var _service = arg__service;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, SchedContextUnbind))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_SchedContext_UnbindObject(arg__service: seL4_SchedContext, arg_cap: seL4_CPtr) callconv(.C) seL4_Error {
    var _service = arg__service;
    var cap = arg_cap;
    var result: seL4_Error = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, SchedContextUnbindObject))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    seL4_SetCap(@as(c_int, 0), cap);
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result = @as(c_uint, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result != @as(c_uint, @bitCast(seL4_NoError))) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
    }
    return result;
}
pub fn seL4_SchedContext_Consumed(arg__service: seL4_SchedContext) callconv(.C) seL4_SchedContext_Consumed_t {
    var _service = arg__service;
    var result: seL4_SchedContext_Consumed_t = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, SchedContextConsumed))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result.@"error" = @as(c_int, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result.@"error" != seL4_NoError) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
        return result;
    }
    result.consumed = mr0;
    return result;
}
pub fn seL4_SchedContext_YieldTo(arg__service: seL4_SchedContext) callconv(.C) seL4_SchedContext_YieldTo_t {
    var _service = arg__service;
    var result: seL4_SchedContext_YieldTo_t = undefined;
    var tag: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, SchedContextYieldTo))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    var output_tag: seL4_MessageInfo_t = undefined;
    var mr0: seL4_Word = undefined;
    var mr1: seL4_Word = undefined;
    var mr2: seL4_Word = undefined;
    var mr3: seL4_Word = undefined;
    mr0 = 0;
    mr1 = 0;
    mr2 = 0;
    mr3 = 0;
    output_tag = seL4_CallWithMRs(_service, tag, &mr0, &mr1, &mr2, &mr3);
    result.@"error" = @as(c_int, @bitCast(@as(c_uint, @truncate(seL4_MessageInfo_get_label(output_tag)))));
    if (result.@"error" != seL4_NoError) {
        seL4_SetMR(@as(c_int, 0), mr0);
        seL4_SetMR(@as(c_int, 1), mr1);
        seL4_SetMR(@as(c_int, 2), mr2);
        seL4_SetMR(@as(c_int, 3), mr3);
        return result;
    }
    result.consumed = mr0;
    return result;
}
pub fn seL4_SchedControl_Configure(arg__service: seL4_SchedControl, arg_schedcontext: seL4_SchedContext, arg_budget: seL4_Time, arg_period: seL4_Time, arg_extra_refills: seL4_Word, arg_badge: seL4_Word) callconv(.C) seL4_Error {
    var _service = arg__service;
    var schedcontext = arg_schedcontext;
    var budget = arg_budget;
    var period = arg_period;
    var extra_refills = arg_extra_refills;
    var badge = arg_badge;
    return seL4_SchedControl_ConfigureFlags(_service, schedcontext, budget, period, extra_refills, badge, @as(seL4_Word, @bitCast(@as(c_long, seL4_SchedContext_NoFlag))));
}
pub const seL4_CapNull: c_int = 0;
pub const seL4_CapInitThreadTCB: c_int = 1;
pub const seL4_CapInitThreadCNode: c_int = 2;
pub const seL4_CapInitThreadVSpace: c_int = 3;
pub const seL4_CapIRQControl: c_int = 4;
pub const seL4_CapASIDControl: c_int = 5;
pub const seL4_CapInitThreadASIDPool: c_int = 6;
pub const seL4_CapIOPortControl: c_int = 7;
pub const seL4_CapIOSpace: c_int = 8;
pub const seL4_CapBootInfoFrame: c_int = 9;
pub const seL4_CapInitThreadIPCBuffer: c_int = 10;
pub const seL4_CapDomain: c_int = 11;
pub const seL4_CapSMMUSIDControl: c_int = 12;
pub const seL4_CapSMMUCBControl: c_int = 13;
pub const seL4_CapInitThreadSC: c_int = 14;
pub const seL4_NumInitialCaps: c_int = 15;
const enum_unnamed_2 = c_uint;
pub const seL4_SlotPos = seL4_Word;
pub const struct_seL4_SlotRegion = extern struct {
    start: seL4_SlotPos,
    end: seL4_SlotPos,
};
pub const seL4_SlotRegion = struct_seL4_SlotRegion;
pub const struct_seL4_UntypedDesc = extern struct {
    paddr: seL4_Word,
    sizeBits: seL4_Uint8,
    isDevice: seL4_Uint8,
    padding: [6]seL4_Uint8,
};
pub const seL4_UntypedDesc = struct_seL4_UntypedDesc; // ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
pub const struct_seL4_BootInfo = extern struct {
    extraLen: seL4_Word,
    nodeID: seL4_NodeId,
    numNodes: seL4_Word,
    numIOPTLevels: seL4_Word,
    ipcBuffer: [*c]seL4_IPCBuffer,
    empty: seL4_SlotRegion,
    sharedFrames: seL4_SlotRegion,
    userImageFrames: seL4_SlotRegion,
    userImagePaging: seL4_SlotRegion,
    ioSpaceCaps: seL4_SlotRegion,
    extraBIPages: seL4_SlotRegion,
    initThreadCNodeSizeBits: seL4_Word,
    initThreadDomain: seL4_Domain,
    schedcontrol: seL4_SlotRegion,
    untyped: seL4_SlotRegion,
    untypedList: [230]seL4_UntypedDesc,
};
pub const seL4_BootInfo = struct_seL4_BootInfo;
pub const SEL4_BOOTINFO_HEADER_PADDING: c_int = 0;
pub const SEL4_BOOTINFO_HEADER_X86_VBE: c_int = 1;
pub const SEL4_BOOTINFO_HEADER_X86_MBMMAP: c_int = 2;
pub const SEL4_BOOTINFO_HEADER_X86_ACPI_RSDP: c_int = 3;
pub const SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER: c_int = 4;
pub const SEL4_BOOTINFO_HEADER_X86_TSC_FREQ: c_int = 5;
pub const SEL4_BOOTINFO_HEADER_FDT: c_int = 6;
pub const SEL4_BOOTINFO_HEADER_NUM: c_int = 7;
pub const _enum_pad_seL4_BootInfoID: c_ulong = 9223372036854775807;
pub const seL4_BootInfoID = c_ulong;
pub const struct_seL4_BootInfoHeader = extern struct {
    id: seL4_Word,
    len: seL4_Word,
};
pub const seL4_BootInfoHeader = struct_seL4_BootInfoHeader; // ./sel4/macros.h:49:43: warning: ignoring StaticAssert declaration
pub extern fn seL4_InitBootInfo(bi: [*c]seL4_BootInfo) void;
pub extern fn seL4_GetBootInfo() [*c]seL4_BootInfo;
pub fn seL4_getArchFault(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Fault_t {
    var tag = arg_tag;
    while (true) {
        switch (seL4_MessageInfo_get_label(tag)) {
            @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 2)))) => return seL4_Fault_UnknownSyscall_new(seL4_GetMR(seL4_UnknownSyscall_X0), seL4_GetMR(seL4_UnknownSyscall_X1), seL4_GetMR(seL4_UnknownSyscall_X2), seL4_GetMR(seL4_UnknownSyscall_X3), seL4_GetMR(seL4_UnknownSyscall_X4), seL4_GetMR(seL4_UnknownSyscall_X5), seL4_GetMR(seL4_UnknownSyscall_X6), seL4_GetMR(seL4_UnknownSyscall_X7), seL4_GetMR(seL4_UnknownSyscall_FaultIP), seL4_GetMR(seL4_UnknownSyscall_SP), seL4_GetMR(seL4_UnknownSyscall_LR), seL4_GetMR(seL4_UnknownSyscall_SPSR), seL4_GetMR(seL4_UnknownSyscall_Syscall)),
            @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 3)))) => return seL4_Fault_UserException_new(seL4_GetMR(seL4_UserException_FaultIP), seL4_GetMR(seL4_UserException_SP), seL4_GetMR(seL4_UserException_SPSR), seL4_GetMR(seL4_UserException_Number), seL4_GetMR(seL4_UserException_Code)),
            @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 6)))) => return seL4_Fault_VMFault_new(seL4_GetMR(seL4_VMFault_IP), seL4_GetMR(seL4_VMFault_Addr), seL4_GetMR(seL4_VMFault_PrefetchFault), seL4_GetMR(seL4_VMFault_FSR)),
            @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 7)))) => return seL4_Fault_VGICMaintenance_new(seL4_GetMR(seL4_VGICMaintenance_IDX)),
            @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 8)))) => return seL4_Fault_VCPUFault_new(seL4_GetMR(seL4_VCPUFault_HSR)),
            @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 9)))) => return seL4_Fault_VPPIEvent_new(seL4_GetMR(seL4_VPPIEvent_IRQ)),
            @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 5)))) => return seL4_Fault_Timeout_new(seL4_GetMR(seL4_Timeout_Data), seL4_GetMR(seL4_Timeout_Consumed)),
            else => return seL4_Fault_NullFault_new(),
        }
        break;
    }
    return @import("std").mem.zeroes(seL4_Fault_t);
}
pub fn seL4_getFault(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Fault_t {
    var tag = arg_tag;
    while (true) {
        switch (seL4_MessageInfo_get_label(tag)) {
            @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 1)))) => return seL4_Fault_CapFault_new(seL4_GetMR(seL4_CapFault_IP), seL4_GetMR(seL4_CapFault_Addr), seL4_GetMR(seL4_CapFault_InRecvPhase), seL4_GetMR(seL4_CapFault_LookupFailureType), seL4_GetMR(seL4_CapFault_BitsLeft), seL4_GetMR(seL4_CapFault_GuardMismatch_GuardFound), seL4_GetMR(seL4_CapFault_GuardMismatch_BitsFound)),
            else => return seL4_getArchFault(tag),
        }
        break;
    }
    return @import("std").mem.zeroes(seL4_Fault_t);
}
pub fn seL4_isVMFault_tag(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Bool {
    var tag = arg_tag;
    return @as(seL4_Bool, @intFromBool(seL4_MessageInfo_get_label(tag) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_VMFault)))));
}
pub fn seL4_isUnknownSyscall_tag(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Bool {
    var tag = arg_tag;
    return @as(seL4_Bool, @intFromBool(seL4_MessageInfo_get_label(tag) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UnknownSyscall)))));
}
pub fn seL4_isUserException_tag(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Bool {
    var tag = arg_tag;
    return @as(seL4_Bool, @intFromBool(seL4_MessageInfo_get_label(tag) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_UserException)))));
}
pub fn seL4_isNullFault_tag(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Bool {
    var tag = arg_tag;
    return @as(seL4_Bool, @intFromBool(seL4_MessageInfo_get_label(tag) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_NullFault)))));
}
pub fn seL4_isCapFault_tag(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Bool {
    var tag = arg_tag;
    return @as(seL4_Bool, @intFromBool(seL4_MessageInfo_get_label(tag) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_CapFault)))));
}
pub fn seL4_isTimeoutFault_tag(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Bool {
    var tag = arg_tag;
    return @as(seL4_Bool, @intFromBool(seL4_MessageInfo_get_label(tag) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_Fault_Timeout)))));
}
pub fn seL4_TimeoutReply_new(arg_resume: seL4_Bool, arg_regs: seL4_UserContext, arg_length: seL4_Word) callconv(.C) seL4_MessageInfo_t {
    var @"resume" = arg_resume;
    var regs = arg_regs;
    var length = arg_length;
    var info: seL4_MessageInfo_t = seL4_MessageInfo_new(@as(seL4_Uint64, @intFromBool(!(@"resume" != 0))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), length);
    {
        var i: seL4_Word = 0;
        while (i < length) : (i +%= 1) {
            seL4_SetMR(@as(c_int, @bitCast(@as(c_uint, @truncate(i)))), @as([*c]seL4_Word, @ptrCast(@alignCast(&regs)))[i]);
        }
    }
    return info;
}
pub const seL4_CapRights = seL4_CapRights_t;
const struct_unnamed_3 = extern struct {
    fault_ip: seL4_Word,
    fault_addr: seL4_Word,
    prefetch_fault: seL4_Word,
    fsr: seL4_Word,
};
pub const seL4_PageFaultIpcRegisters = extern union {
    regs: struct_unnamed_3,
    raw: [4]seL4_Word,
};
pub const seL4_FaultType = seL4_Fault_tag_t;
pub fn seL4_GetTag() callconv(.C) seL4_MessageInfo_t {
    return seL4_GetIPCBuffer().*.tag;
}
pub fn seL4_SetTag(arg_tag: seL4_MessageInfo_t) callconv(.C) void {
    var tag = arg_tag;
    seL4_GetIPCBuffer().*.tag = tag;
}
pub fn seL4_PF_FIP() callconv(.C) seL4_Word {
    return seL4_GetMR(seL4_VMFault_IP);
}
pub fn seL4_PF_Addr() callconv(.C) seL4_Word {
    return seL4_GetMR(seL4_VMFault_Addr);
}
pub fn seL4_isPageFault_MSG() callconv(.C) seL4_Word {
    return @as(seL4_Word, @bitCast(@as(c_long, seL4_isVMFault_tag(seL4_GetIPCBuffer().*.tag))));
}
pub fn seL4_isPageFault_Tag(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Word {
    var tag = arg_tag;
    return @as(seL4_Word, @bitCast(@as(c_long, seL4_isVMFault_tag(tag))));
}
pub fn seL4_isExceptIPC_Tag(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Word {
    var tag = arg_tag;
    return @as(seL4_Word, @bitCast(@as(c_long, seL4_isUnknownSyscall_tag(tag))));
}
pub fn seL4_ExceptIPC_Get(arg_mr: seL4_Word) callconv(.C) seL4_Word {
    var mr = arg_mr;
    return seL4_GetMR(@as(c_int, @bitCast(@as(c_uint, @truncate(mr)))));
}
pub fn seL4_ExceptIPC_Set(arg_index: seL4_Word, arg_val: seL4_Word) callconv(.C) void {
    var index = arg_index;
    var val = arg_val;
    seL4_SetMR(@as(c_int, @bitCast(@as(c_uint, @truncate(index)))), val);
}
pub fn seL4_IsArchSyscallFrom(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Word {
    var tag = arg_tag;
    return @as(seL4_Word, @intFromBool(seL4_MessageInfo_get_length(tag) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_UnknownSyscall_Length)))));
}
pub fn seL4_IsArchExceptionFrom(arg_tag: seL4_MessageInfo_t) callconv(.C) seL4_Word {
    var tag = arg_tag;
    return @as(seL4_Word, @intFromBool(seL4_MessageInfo_get_length(tag) == @as(seL4_Uint64, @bitCast(@as(c_long, seL4_UnknownSyscall_Length)))));
}
pub const seL4_CapData_t = seL4_Word;
pub fn seL4_CapData_Badge_new(arg_badge: seL4_Word) callconv(.C) seL4_Word {
    var badge = arg_badge;
    return badge;
}
pub fn seL4_CapData_Guard_new(arg_guard: seL4_Word, arg_bits: seL4_Word) callconv(.C) seL4_Word {
    var guard = arg_guard;
    var bits = arg_bits;
    return seL4_CNode_CapData_new(guard, bits).words[@as(c_uint, @intCast(@as(c_int, 0)))];
}
pub const sel4cp_channel = c_uint;
pub const sel4cp_id = c_uint;
pub const sel4cp_msginfo = seL4_MessageInfo_t;
pub extern fn init() void;
pub extern fn notified(ch: sel4cp_channel) void;
pub extern fn protected(ch: sel4cp_channel, msginfo: sel4cp_msginfo) sel4cp_msginfo;
pub extern fn fault(ch: sel4cp_channel, msginfo: sel4cp_msginfo) void;
pub extern var sel4cp_name: [16]u8;
pub extern var have_signal: bool;
pub extern var signal: seL4_CPtr;
pub extern var signal_msg: seL4_MessageInfo_t;
pub extern fn sel4cp_dbg_putc(c: c_int) void;
pub extern fn sel4cp_dbg_puts(s: [*c]const u8) void;
pub fn memzero(arg_s: ?*anyopaque, arg_n: c_ulong) callconv(.C) void {
    var s = arg_s;
    var n = arg_n;
    var p: [*c]u8 = undefined;
    {
        p = @as([*c]u8, @ptrCast(@alignCast(s)));
        while (n > @as(c_ulong, @bitCast(@as(c_long, @as(c_int, 0))))) : (_ = blk: {
            n -%= 1;
            break :blk blk_1: {
                const ref = &p;
                const tmp = ref.*;
                ref.* += 1;
                break :blk_1 tmp;
            };
        }) {
            p.* = 0;
        }
    }
}
pub fn sel4cp_internal_crash(arg_err: seL4_Error) callconv(.C) void {
    var err = arg_err;
    var x: [*c]c_int = @as([*c]c_int, @ptrFromInt(@as(usize, @bitCast(@as(c_ulong, err)))));
    x.* = 0;
}
pub fn sel4cp_notify(arg_ch: sel4cp_channel) callconv(.C) void {
    var ch = arg_ch;
    seL4_Signal(@as(seL4_CPtr, @bitCast(@as(c_ulong, @as(sel4cp_channel, @bitCast(@as(c_int, 10))) +% ch))));
}
pub fn sel4cp_irq_ack(arg_ch: sel4cp_channel) callconv(.C) void {
    var ch = arg_ch;
    _ = seL4_IRQHandler_Ack(@as(seL4_IRQHandler, @bitCast(@as(c_ulong, @as(sel4cp_channel, @bitCast(@as(c_int, 138))) +% ch))));
}
pub fn sel4cp_notify_delayed(arg_ch: sel4cp_channel) callconv(.C) void {
    var ch = arg_ch;
    have_signal = @as(c_int, 1) != 0;
    signal_msg = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    signal = @as(seL4_CPtr, @bitCast(@as(c_ulong, @as(sel4cp_channel, @bitCast(@as(c_int, 10))) +% ch)));
}
pub fn sel4cp_irq_ack_delayed(arg_ch: sel4cp_channel) callconv(.C) void {
    var ch = arg_ch;
    have_signal = @as(c_int, 1) != 0;
    signal_msg = seL4_MessageInfo_new(@as(seL4_Uint64, @bitCast(@as(c_long, IRQAckIRQ))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))));
    signal = @as(seL4_CPtr, @bitCast(@as(c_ulong, @as(sel4cp_channel, @bitCast(@as(c_int, 138))) +% ch)));
}
pub fn sel4cp_pd_restart(arg_pd: sel4cp_id, arg_entry_point: usize) callconv(.C) void {
    var pd = arg_pd;
    var entry_point = arg_entry_point;
    var err: seL4_Error = undefined;
    var ctxt: seL4_UserContext = undefined;
    memzero(@as(?*anyopaque, @ptrCast(&ctxt)), @sizeOf(seL4_UserContext));
    ctxt.pc = entry_point;
    err = seL4_TCB_WriteRegisters(@as(seL4_TCB, @bitCast(@as(c_ulong, @as(sel4cp_id, @bitCast(@as(c_int, 202))) +% pd))), @as(seL4_Bool, @bitCast(@as(i8, @truncate(@as(c_int, 1))))), @as(seL4_Uint8, @bitCast(@as(i8, @truncate(@as(c_int, 0))))), @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 1)))), &ctxt);
    if (err != @as(c_uint, @bitCast(seL4_NoError))) {
        sel4cp_dbg_puts("sel4cp_pd_restart: error writing TCB registers\n");
        sel4cp_internal_crash(err);
    }
}
pub fn sel4cp_pd_stop(arg_pd: sel4cp_id) callconv(.C) void {
    var pd = arg_pd;
    var err: seL4_Error = undefined;
    err = seL4_TCB_Suspend(@as(seL4_TCB, @bitCast(@as(c_ulong, @as(sel4cp_id, @bitCast(@as(c_int, 202))) +% pd))));
    if (err != @as(c_uint, @bitCast(seL4_NoError))) {
        sel4cp_dbg_puts("sel4cp_pd_stop: error suspending TCB\n");
        sel4cp_internal_crash(err);
    }
}
pub fn sel4cp_fault_reply(arg_msginfo: sel4cp_msginfo) callconv(.C) void {
    var msginfo = arg_msginfo;
    seL4_Send(@as(seL4_CPtr, @bitCast(@as(c_long, @as(c_int, 4)))), msginfo);
}
pub fn sel4cp_ppcall(arg_ch: sel4cp_channel, arg_msginfo: sel4cp_msginfo) callconv(.C) sel4cp_msginfo {
    var ch = arg_ch;
    var msginfo = arg_msginfo;
    return seL4_Call(@as(seL4_CPtr, @bitCast(@as(c_ulong, @as(sel4cp_channel, @bitCast(@as(c_int, 74))) +% ch))), msginfo);
}
pub fn sel4cp_msginfo_new(arg_label: u64, arg_count: u16) callconv(.C) sel4cp_msginfo {
    var label = arg_label;
    var count = arg_count;
    return seL4_MessageInfo_new(label, @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_long, @as(c_int, 0)))), @as(seL4_Uint64, @bitCast(@as(c_ulong, count))));
}
pub fn sel4cp_msginfo_get_label(arg_msginfo: sel4cp_msginfo) callconv(.C) u64 {
    var msginfo = arg_msginfo;
    return seL4_MessageInfo_get_label(msginfo);
}
pub fn sel4cp_mr_set(arg_mr: u8, arg_value: u64) callconv(.C) void {
    var mr = arg_mr;
    var value = arg_value;
    seL4_SetMR(@as(c_int, @bitCast(@as(c_uint, mr))), value);
}
pub fn sel4cp_mr_get(arg_mr: u8) callconv(.C) u64 {
    var mr = arg_mr;
    return seL4_GetMR(@as(c_int, @bitCast(@as(c_uint, mr))));
}
pub fn sel4cp_vm_restart(arg_vm: sel4cp_id, arg_entry_point: usize) callconv(.C) void {
    var vm = arg_vm;
    var entry_point = arg_entry_point;
    var err: seL4_Error = undefined;
    var ctxt: seL4_UserContext = undefined;
    memzero(@as(?*anyopaque, @ptrCast(&ctxt)), @sizeOf(seL4_UserContext));
    ctxt.pc = entry_point;
    err = seL4_TCB_WriteRegisters(@as(seL4_TCB, @bitCast(@as(c_ulong, @as(sel4cp_id, @bitCast(@as(c_int, 266))) +% vm))), @as(seL4_Bool, @bitCast(@as(i8, @truncate(@as(c_int, 1))))), @as(seL4_Uint8, @bitCast(@as(i8, @truncate(@as(c_int, 0))))), @as(seL4_Word, @bitCast(@as(c_long, @as(c_int, 1)))), &ctxt);
    if (err != @as(c_uint, @bitCast(seL4_NoError))) {
        sel4cp_dbg_puts("sel4cp_vm_restart: error writing registers\n");
        sel4cp_internal_crash(err);
    }
}
pub fn sel4cp_vm_stop(arg_vm: sel4cp_id) callconv(.C) void {
    var vm = arg_vm;
    var err: seL4_Error = undefined;
    err = seL4_TCB_Suspend(@as(seL4_TCB, @bitCast(@as(c_ulong, @as(sel4cp_id, @bitCast(@as(c_int, 266))) +% vm))));
    if (err != @as(c_uint, @bitCast(seL4_NoError))) {
        sel4cp_dbg_puts("sel4cp_vm_stop: error suspending TCB\n");
        sel4cp_internal_crash(err);
    }
}
pub fn sel4cp_arm_vcpu_inject_irq(arg_vm: sel4cp_id, arg_irq: u16, arg_priority: u8, arg_group: u8, arg_index: u8) callconv(.C) void {
    var vm = arg_vm;
    var irq = arg_irq;
    var priority = arg_priority;
    var group = arg_group;
    var index = arg_index;
    var err: seL4_Error = undefined;
    err = seL4_ARM_VCPU_InjectIRQ(@as(seL4_ARM_VCPU, @bitCast(@as(c_ulong, @as(sel4cp_id, @bitCast(@as(c_int, 330))) +% vm))), irq, priority, group, index);
    if (err != @as(c_uint, @bitCast(seL4_NoError))) {
        sel4cp_dbg_puts("sel4cp_arm_vcpu_inject_irq: error injecting IRQ\n");
        sel4cp_internal_crash(err);
    }
}
pub fn sel4cp_arm_vcpu_ack_vppi(arg_vm: sel4cp_id, arg_irq: u64) callconv(.C) void {
    var vm = arg_vm;
    var irq = arg_irq;
    var err: seL4_Error = undefined;
    err = seL4_ARM_VCPU_AckVPPI(@as(seL4_ARM_VCPU, @bitCast(@as(c_ulong, @as(sel4cp_id, @bitCast(@as(c_int, 330))) +% vm))), irq);
    if (err != @as(c_uint, @bitCast(seL4_NoError))) {
        sel4cp_dbg_puts("sel4cp_arm_vcpu_ack_vppi: error acking VPPI\n");
        sel4cp_internal_crash(err);
    }
}
pub fn sel4cp_arm_vcpu_read_reg(arg_vm: sel4cp_id, arg_reg: u64) callconv(.C) seL4_Word {
    var vm = arg_vm;
    var reg = arg_reg;
    var ret: seL4_ARM_VCPU_ReadRegs_t = undefined;
    ret = seL4_ARM_VCPU_ReadRegs(@as(seL4_ARM_VCPU, @bitCast(@as(c_ulong, @as(sel4cp_id, @bitCast(@as(c_int, 330))) +% vm))), reg);
    if (ret.@"error" != seL4_NoError) {
        sel4cp_dbg_puts("sel4cp_arm_vcpu_read_reg: error reading VCPU register\n");
        sel4cp_internal_crash(@as(c_uint, @bitCast(ret.@"error")));
    }
    return ret.value;
}
pub fn sel4cp_arm_vcpu_write_reg(arg_vm: sel4cp_id, arg_reg: u64, arg_value: u64) callconv(.C) void {
    var vm = arg_vm;
    var reg = arg_reg;
    var value = arg_value;
    var err: seL4_Error = undefined;
    err = seL4_ARM_VCPU_WriteRegs(@as(seL4_ARM_VCPU, @bitCast(@as(c_ulong, @as(sel4cp_id, @bitCast(@as(c_int, 330))) +% vm))), reg, value);
    if (err != @as(c_uint, @bitCast(seL4_NoError))) {
        sel4cp_dbg_puts("sel4cp_arm_vcpu_write_reg: error VPPI\n");
        sel4cp_internal_crash(err);
    }
}
pub const __INTMAX_C_SUFFIX__ = @compileError("unable to translate macro: undefined identifier `L`"); // (no file):80:9
pub const __UINTMAX_C_SUFFIX__ = @compileError("unable to translate macro: undefined identifier `UL`"); // (no file):86:9
pub const __FLT16_DENORM_MIN__ = @compileError("unable to translate C expr: unexpected token 'IntegerLiteral'"); // (no file):109:9
pub const __FLT16_EPSILON__ = @compileError("unable to translate C expr: unexpected token 'IntegerLiteral'"); // (no file):113:9
pub const __FLT16_MAX__ = @compileError("unable to translate C expr: unexpected token 'IntegerLiteral'"); // (no file):119:9
pub const __FLT16_MIN__ = @compileError("unable to translate C expr: unexpected token 'IntegerLiteral'"); // (no file):122:9
pub const __INT64_C_SUFFIX__ = @compileError("unable to translate macro: undefined identifier `L`"); // (no file):184:9
pub const __UINT32_C_SUFFIX__ = @compileError("unable to translate macro: undefined identifier `U`"); // (no file):206:9
pub const __UINT64_C_SUFFIX__ = @compileError("unable to translate macro: undefined identifier `UL`"); // (no file):214:9
pub const __stdint_join3 = @compileError("unable to translate C expr: unexpected token '##'"); // /Users/ivanv/zigs/zig-macos-x86_64-0.11.0-dev.3942+49ac816e3/lib/include/stdint.h:287:9
pub const __int_c_join = @compileError("unable to translate C expr: unexpected token '##'"); // /Users/ivanv/zigs/zig-macos-x86_64-0.11.0-dev.3942+49ac816e3/lib/include/stdint.h:324:9
pub const __uint_c = @compileError("unable to translate macro: undefined identifier `U`"); // /Users/ivanv/zigs/zig-macos-x86_64-0.11.0-dev.3942+49ac816e3/lib/include/stdint.h:326:9
pub const __INTN_MIN = @compileError("unable to translate macro: undefined identifier `INT`"); // /Users/ivanv/zigs/zig-macos-x86_64-0.11.0-dev.3942+49ac816e3/lib/include/stdint.h:894:10
pub const __INTN_MAX = @compileError("unable to translate macro: undefined identifier `INT`"); // /Users/ivanv/zigs/zig-macos-x86_64-0.11.0-dev.3942+49ac816e3/lib/include/stdint.h:895:10
pub const __UINTN_MAX = @compileError("unable to translate macro: undefined identifier `UINT`"); // /Users/ivanv/zigs/zig-macos-x86_64-0.11.0-dev.3942+49ac816e3/lib/include/stdint.h:896:9
pub const __INTN_C = @compileError("unable to translate macro: undefined identifier `INT`"); // /Users/ivanv/zigs/zig-macos-x86_64-0.11.0-dev.3942+49ac816e3/lib/include/stdint.h:897:10
pub const __UINTN_C = @compileError("unable to translate macro: undefined identifier `UINT`"); // /Users/ivanv/zigs/zig-macos-x86_64-0.11.0-dev.3942+49ac816e3/lib/include/stdint.h:898:9
pub const CONFIG_SEL4_ARCH = @compileError("unable to translate macro: undefined identifier `aarch64`"); // ./kernel/gen_config.h:16:9
pub const CONFIG_ARCH = @compileError("unable to translate macro: undefined identifier `arm`"); // ./kernel/gen_config.h:18:9
pub const CONFIG_ARM_PLAT = @compileError("unable to translate macro: undefined identifier `qemu`"); // ./kernel/gen_config.h:20:9
pub const CONFIG_PLAT = @compileError("unable to translate macro: undefined identifier `qemu`"); // ./kernel/gen_config.h:38:9
pub const CONFIG_KERNEL_BENCHMARK = @compileError("unable to translate macro: undefined identifier `none`"); // ./kernel/gen_config.h:109:9
pub const CONFIG_KERNEL_OPT_LEVEL = @compileError("unable to translate macro: undefined identifier `O2`"); // ./kernel/gen_config.h:121:9
pub const CONFIG_LIB_SEL4_FUNCTION_ATTRIBUTE = @compileError("unable to translate C expr: unexpected token 'inline'"); // ./sel4/gen_config.h:6:9
pub const CONST = @compileError("unable to translate macro: undefined identifier `__attribute__`"); // ./sel4/macros.h:12:9
pub const PURE = @compileError("unable to translate macro: undefined identifier `__attribute__`"); // ./sel4/macros.h:16:9
pub const SEL4_PACKED = @compileError("unable to translate macro: undefined identifier `__attribute__`"); // ./sel4/macros.h:19:9
pub const SEL4_DEPRECATED = @compileError("unable to translate macro: undefined identifier `__attribute__`"); // ./sel4/macros.h:20:9
pub const SEL4_DEPRECATE_MACRO = @compileError("unable to translate macro: undefined identifier `_Pragma`"); // ./sel4/macros.h:21:9
pub const SEL4_OFFSETOF = @compileError("unable to translate macro: undefined identifier `__builtin_offsetof`"); // ./sel4/macros.h:22:9
pub const LIBSEL4_UNUSED = @compileError("unable to translate macro: undefined identifier `__attribute__`"); // ./sel4/macros.h:24:9
pub const LIBSEL4_WEAK = @compileError("unable to translate macro: undefined identifier `__attribute__`"); // ./sel4/macros.h:25:9
pub const LIBSEL4_NOINLINE = @compileError("unable to translate macro: undefined identifier `__attribute__`"); // ./sel4/macros.h:26:9
pub const LIBSEL4_INLINE = @compileError("unable to translate C expr: unexpected token 'static'"); // ./sel4/macros.h:31:9
pub const LIBSEL4_INLINE_FUNC = @compileError("unable to translate C expr: unexpected token 'static'"); // ./sel4/macros.h:32:9
pub const SEL4_COMPILE_ASSERT = @compileError("unable to translate C expr: unexpected token '_Static_assert'"); // ./sel4/macros.h:49:9
pub const SEL4_SIZE_SANITY = @compileError("unable to translate macro: undefined identifier `_`"); // ./sel4/macros.h:56:9
pub const SEL4_FORCE_LONG_ENUM = @compileError("unable to translate macro: undefined identifier `_enum_pad_`"); // ./sel4/macros.h:65:9
pub const seL4_integer_size_assert = @compileError("unable to translate macro: undefined identifier `sizeof_`"); // ./sel4/simple_types.h:29:9
pub const _seL4_int64_fmt = @compileError("unable to translate macro: undefined identifier `l`"); // ./sel4/simple_types.h:75:9
pub const _macro_str_concat_helper2 = @compileError("unable to translate C expr: unexpected token '#'"); // ./sel4/simple_types.h:90:9
pub const _macro_str_concat_helper1 = @compileError("unable to translate C expr: expected ',' or ')' instead got '##'"); // ./sel4/simple_types.h:91:9
pub const SEL4_PRId64 = @compileError("unable to translate macro: undefined identifier `d`"); // ./sel4/simple_types.h:94:9
pub const SEL4_PRIi64 = @compileError("unable to translate macro: undefined identifier `i`"); // ./sel4/simple_types.h:95:9
pub const SEL4_PRIu64 = @compileError("unable to translate macro: undefined identifier `u`"); // ./sel4/simple_types.h:96:9
pub const SEL4_PRIx64 = @compileError("unable to translate macro: undefined identifier `x`"); // ./sel4/simple_types.h:97:9
pub const SEL4_PRId_word = @compileError("unable to translate macro: undefined identifier `d`"); // ./sel4/simple_types.h:122:9
pub const SEL4_PRIi_word = @compileError("unable to translate macro: undefined identifier `i`"); // ./sel4/simple_types.h:123:9
pub const SEL4_PRIu_word = @compileError("unable to translate macro: undefined identifier `u`"); // ./sel4/simple_types.h:124:9
pub const SEL4_PRIx_word = @compileError("unable to translate macro: undefined identifier `x`"); // ./sel4/simple_types.h:125:9
pub const seL4_Fail = @compileError("unable to translate macro: undefined identifier `__FILE__`"); // ./sel4/assert.h:28:9
pub const seL4_Assert = @compileError("unable to translate macro: undefined identifier `__FILE__`"); // ./sel4/assert.h:34:9
pub const MCS_PARAM_DECL = @compileError("unable to translate macro: undefined identifier `reply_reg`"); // ./sel4/sel4_arch/syscalls.h:14:9
pub const MCS_PARAM = @compileError("unable to translate macro: undefined identifier `reply_reg`"); // ./sel4/sel4_arch/syscalls.h:15:9
pub const LIBSEL4_MCS_REPLY = @compileError("unable to translate macro: undefined identifier `reply`"); // ./sel4/arch/syscalls.h:15:9
pub const assert_size_correct = @compileError("unable to translate macro: undefined identifier `__type_`"); // ./interfaces/sel4_client.h:18:9
pub const __llvm__ = @as(c_int, 1);
pub const __clang__ = @as(c_int, 1);
pub const __clang_major__ = @as(c_int, 16);
pub const __clang_minor__ = @as(c_int, 0);
pub const __clang_patchlevel__ = @as(c_int, 1);
pub const __clang_version__ = "16.0.1 (https://github.com/ziglang/zig-bootstrap bf1b2cdb83141ad9336eec42160c9fe87f90198d)";
pub const __GNUC__ = @as(c_int, 4);
pub const __GNUC_MINOR__ = @as(c_int, 2);
pub const __GNUC_PATCHLEVEL__ = @as(c_int, 1);
pub const __GXX_ABI_VERSION = @as(c_int, 1002);
pub const __ATOMIC_RELAXED = @as(c_int, 0);
pub const __ATOMIC_CONSUME = @as(c_int, 1);
pub const __ATOMIC_ACQUIRE = @as(c_int, 2);
pub const __ATOMIC_RELEASE = @as(c_int, 3);
pub const __ATOMIC_ACQ_REL = @as(c_int, 4);
pub const __ATOMIC_SEQ_CST = @as(c_int, 5);
pub const __OPENCL_MEMORY_SCOPE_WORK_ITEM = @as(c_int, 0);
pub const __OPENCL_MEMORY_SCOPE_WORK_GROUP = @as(c_int, 1);
pub const __OPENCL_MEMORY_SCOPE_DEVICE = @as(c_int, 2);
pub const __OPENCL_MEMORY_SCOPE_ALL_SVM_DEVICES = @as(c_int, 3);
pub const __OPENCL_MEMORY_SCOPE_SUB_GROUP = @as(c_int, 4);
pub const __PRAGMA_REDEFINE_EXTNAME = @as(c_int, 1);
pub const __VERSION__ = "Clang 16.0.1 (https://github.com/ziglang/zig-bootstrap bf1b2cdb83141ad9336eec42160c9fe87f90198d)";
pub const __OBJC_BOOL_IS_BOOL = @as(c_int, 0);
pub const __CONSTANT_CFSTRINGS__ = @as(c_int, 1);
pub const __clang_literal_encoding__ = "UTF-8";
pub const __clang_wide_literal_encoding__ = "UTF-32";
pub const __ORDER_LITTLE_ENDIAN__ = @as(c_int, 1234);
pub const __ORDER_BIG_ENDIAN__ = @as(c_int, 4321);
pub const __ORDER_PDP_ENDIAN__ = @as(c_int, 3412);
pub const __BYTE_ORDER__ = __ORDER_LITTLE_ENDIAN__;
pub const __LITTLE_ENDIAN__ = @as(c_int, 1);
pub const _LP64 = @as(c_int, 1);
pub const __LP64__ = @as(c_int, 1);
pub const __CHAR_BIT__ = @as(c_int, 8);
pub const __BOOL_WIDTH__ = @as(c_int, 8);
pub const __SHRT_WIDTH__ = @as(c_int, 16);
pub const __INT_WIDTH__ = @as(c_int, 32);
pub const __LONG_WIDTH__ = @as(c_int, 64);
pub const __LLONG_WIDTH__ = @as(c_int, 64);
pub const __BITINT_MAXWIDTH__ = @as(c_int, 128);
pub const __SCHAR_MAX__ = @as(c_int, 127);
pub const __SHRT_MAX__ = @as(c_int, 32767);
pub const __INT_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 2147483647, .decimal);
pub const __LONG_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_long, 9223372036854775807, .decimal);
pub const __LONG_LONG_MAX__ = @as(c_longlong, 9223372036854775807);
pub const __WCHAR_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_uint, 4294967295, .decimal);
pub const __WCHAR_WIDTH__ = @as(c_int, 32);
pub const __WINT_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 2147483647, .decimal);
pub const __WINT_WIDTH__ = @as(c_int, 32);
pub const __INTMAX_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_long, 9223372036854775807, .decimal);
pub const __INTMAX_WIDTH__ = @as(c_int, 64);
pub const __SIZE_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_ulong, 18446744073709551615, .decimal);
pub const __SIZE_WIDTH__ = @as(c_int, 64);
pub const __UINTMAX_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_ulong, 18446744073709551615, .decimal);
pub const __UINTMAX_WIDTH__ = @as(c_int, 64);
pub const __PTRDIFF_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_long, 9223372036854775807, .decimal);
pub const __PTRDIFF_WIDTH__ = @as(c_int, 64);
pub const __INTPTR_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_long, 9223372036854775807, .decimal);
pub const __INTPTR_WIDTH__ = @as(c_int, 64);
pub const __UINTPTR_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_ulong, 18446744073709551615, .decimal);
pub const __UINTPTR_WIDTH__ = @as(c_int, 64);
pub const __SIZEOF_DOUBLE__ = @as(c_int, 8);
pub const __SIZEOF_FLOAT__ = @as(c_int, 4);
pub const __SIZEOF_INT__ = @as(c_int, 4);
pub const __SIZEOF_LONG__ = @as(c_int, 8);
pub const __SIZEOF_LONG_DOUBLE__ = @as(c_int, 16);
pub const __SIZEOF_LONG_LONG__ = @as(c_int, 8);
pub const __SIZEOF_POINTER__ = @as(c_int, 8);
pub const __SIZEOF_SHORT__ = @as(c_int, 2);
pub const __SIZEOF_PTRDIFF_T__ = @as(c_int, 8);
pub const __SIZEOF_SIZE_T__ = @as(c_int, 8);
pub const __SIZEOF_WCHAR_T__ = @as(c_int, 4);
pub const __SIZEOF_WINT_T__ = @as(c_int, 4);
pub const __SIZEOF_INT128__ = @as(c_int, 16);
pub const __INTMAX_TYPE__ = c_long;
pub const __INTMAX_FMTd__ = "ld";
pub const __INTMAX_FMTi__ = "li";
pub const __UINTMAX_TYPE__ = c_ulong;
pub const __UINTMAX_FMTo__ = "lo";
pub const __UINTMAX_FMTu__ = "lu";
pub const __UINTMAX_FMTx__ = "lx";
pub const __UINTMAX_FMTX__ = "lX";
pub const __PTRDIFF_TYPE__ = c_long;
pub const __PTRDIFF_FMTd__ = "ld";
pub const __PTRDIFF_FMTi__ = "li";
pub const __INTPTR_TYPE__ = c_long;
pub const __INTPTR_FMTd__ = "ld";
pub const __INTPTR_FMTi__ = "li";
pub const __SIZE_TYPE__ = c_ulong;
pub const __SIZE_FMTo__ = "lo";
pub const __SIZE_FMTu__ = "lu";
pub const __SIZE_FMTx__ = "lx";
pub const __SIZE_FMTX__ = "lX";
pub const __WCHAR_TYPE__ = c_uint;
pub const __WINT_TYPE__ = c_int;
pub const __SIG_ATOMIC_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 2147483647, .decimal);
pub const __SIG_ATOMIC_WIDTH__ = @as(c_int, 32);
pub const __CHAR16_TYPE__ = c_ushort;
pub const __CHAR32_TYPE__ = c_uint;
pub const __UINTPTR_TYPE__ = c_ulong;
pub const __UINTPTR_FMTo__ = "lo";
pub const __UINTPTR_FMTu__ = "lu";
pub const __UINTPTR_FMTx__ = "lx";
pub const __UINTPTR_FMTX__ = "lX";
pub const __FLT16_HAS_DENORM__ = @as(c_int, 1);
pub const __FLT16_DIG__ = @as(c_int, 3);
pub const __FLT16_DECIMAL_DIG__ = @as(c_int, 5);
pub const __FLT16_HAS_INFINITY__ = @as(c_int, 1);
pub const __FLT16_HAS_QUIET_NAN__ = @as(c_int, 1);
pub const __FLT16_MANT_DIG__ = @as(c_int, 11);
pub const __FLT16_MAX_10_EXP__ = @as(c_int, 4);
pub const __FLT16_MAX_EXP__ = @as(c_int, 16);
pub const __FLT16_MIN_10_EXP__ = -@as(c_int, 4);
pub const __FLT16_MIN_EXP__ = -@as(c_int, 13);
pub const __FLT_DENORM_MIN__ = @as(f32, 1.40129846e-45);
pub const __FLT_HAS_DENORM__ = @as(c_int, 1);
pub const __FLT_DIG__ = @as(c_int, 6);
pub const __FLT_DECIMAL_DIG__ = @as(c_int, 9);
pub const __FLT_EPSILON__ = @as(f32, 1.19209290e-7);
pub const __FLT_HAS_INFINITY__ = @as(c_int, 1);
pub const __FLT_HAS_QUIET_NAN__ = @as(c_int, 1);
pub const __FLT_MANT_DIG__ = @as(c_int, 24);
pub const __FLT_MAX_10_EXP__ = @as(c_int, 38);
pub const __FLT_MAX_EXP__ = @as(c_int, 128);
pub const __FLT_MAX__ = @as(f32, 3.40282347e+38);
pub const __FLT_MIN_10_EXP__ = -@as(c_int, 37);
pub const __FLT_MIN_EXP__ = -@as(c_int, 125);
pub const __FLT_MIN__ = @as(f32, 1.17549435e-38);
pub const __DBL_DENORM_MIN__ = @as(f64, 4.9406564584124654e-324);
pub const __DBL_HAS_DENORM__ = @as(c_int, 1);
pub const __DBL_DIG__ = @as(c_int, 15);
pub const __DBL_DECIMAL_DIG__ = @as(c_int, 17);
pub const __DBL_EPSILON__ = @as(f64, 2.2204460492503131e-16);
pub const __DBL_HAS_INFINITY__ = @as(c_int, 1);
pub const __DBL_HAS_QUIET_NAN__ = @as(c_int, 1);
pub const __DBL_MANT_DIG__ = @as(c_int, 53);
pub const __DBL_MAX_10_EXP__ = @as(c_int, 308);
pub const __DBL_MAX_EXP__ = @as(c_int, 1024);
pub const __DBL_MAX__ = @as(f64, 1.7976931348623157e+308);
pub const __DBL_MIN_10_EXP__ = -@as(c_int, 307);
pub const __DBL_MIN_EXP__ = -@as(c_int, 1021);
pub const __DBL_MIN__ = @as(f64, 2.2250738585072014e-308);
pub const __LDBL_DENORM_MIN__ = @as(c_longdouble, 6.47517511943802511092443895822764655e-4966);
pub const __LDBL_HAS_DENORM__ = @as(c_int, 1);
pub const __LDBL_DIG__ = @as(c_int, 33);
pub const __LDBL_DECIMAL_DIG__ = @as(c_int, 36);
pub const __LDBL_EPSILON__ = @as(c_longdouble, 1.92592994438723585305597794258492732e-34);
pub const __LDBL_HAS_INFINITY__ = @as(c_int, 1);
pub const __LDBL_HAS_QUIET_NAN__ = @as(c_int, 1);
pub const __LDBL_MANT_DIG__ = @as(c_int, 113);
pub const __LDBL_MAX_10_EXP__ = @as(c_int, 4932);
pub const __LDBL_MAX_EXP__ = @as(c_int, 16384);
pub const __LDBL_MAX__ = @as(c_longdouble, 1.18973149535723176508575932662800702e+4932);
pub const __LDBL_MIN_10_EXP__ = -@as(c_int, 4931);
pub const __LDBL_MIN_EXP__ = -@as(c_int, 16381);
pub const __LDBL_MIN__ = @as(c_longdouble, 3.36210314311209350626267781732175260e-4932);
pub const __POINTER_WIDTH__ = @as(c_int, 64);
pub const __BIGGEST_ALIGNMENT__ = @as(c_int, 16);
pub const __CHAR_UNSIGNED__ = @as(c_int, 1);
pub const __WCHAR_UNSIGNED__ = @as(c_int, 1);
pub const __INT8_TYPE__ = i8;
pub const __INT8_FMTd__ = "hhd";
pub const __INT8_FMTi__ = "hhi";
pub const __INT8_C_SUFFIX__ = "";
pub const __INT16_TYPE__ = c_short;
pub const __INT16_FMTd__ = "hd";
pub const __INT16_FMTi__ = "hi";
pub const __INT16_C_SUFFIX__ = "";
pub const __INT32_TYPE__ = c_int;
pub const __INT32_FMTd__ = "d";
pub const __INT32_FMTi__ = "i";
pub const __INT32_C_SUFFIX__ = "";
pub const __INT64_TYPE__ = c_long;
pub const __INT64_FMTd__ = "ld";
pub const __INT64_FMTi__ = "li";
pub const __UINT8_TYPE__ = u8;
pub const __UINT8_FMTo__ = "hho";
pub const __UINT8_FMTu__ = "hhu";
pub const __UINT8_FMTx__ = "hhx";
pub const __UINT8_FMTX__ = "hhX";
pub const __UINT8_C_SUFFIX__ = "";
pub const __UINT8_MAX__ = @as(c_int, 255);
pub const __INT8_MAX__ = @as(c_int, 127);
pub const __UINT16_TYPE__ = c_ushort;
pub const __UINT16_FMTo__ = "ho";
pub const __UINT16_FMTu__ = "hu";
pub const __UINT16_FMTx__ = "hx";
pub const __UINT16_FMTX__ = "hX";
pub const __UINT16_C_SUFFIX__ = "";
pub const __UINT16_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 65535, .decimal);
pub const __INT16_MAX__ = @as(c_int, 32767);
pub const __UINT32_TYPE__ = c_uint;
pub const __UINT32_FMTo__ = "o";
pub const __UINT32_FMTu__ = "u";
pub const __UINT32_FMTx__ = "x";
pub const __UINT32_FMTX__ = "X";
pub const __UINT32_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_uint, 4294967295, .decimal);
pub const __INT32_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 2147483647, .decimal);
pub const __UINT64_TYPE__ = c_ulong;
pub const __UINT64_FMTo__ = "lo";
pub const __UINT64_FMTu__ = "lu";
pub const __UINT64_FMTx__ = "lx";
pub const __UINT64_FMTX__ = "lX";
pub const __UINT64_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_ulong, 18446744073709551615, .decimal);
pub const __INT64_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_long, 9223372036854775807, .decimal);
pub const __INT_LEAST8_TYPE__ = i8;
pub const __INT_LEAST8_MAX__ = @as(c_int, 127);
pub const __INT_LEAST8_WIDTH__ = @as(c_int, 8);
pub const __INT_LEAST8_FMTd__ = "hhd";
pub const __INT_LEAST8_FMTi__ = "hhi";
pub const __UINT_LEAST8_TYPE__ = u8;
pub const __UINT_LEAST8_MAX__ = @as(c_int, 255);
pub const __UINT_LEAST8_FMTo__ = "hho";
pub const __UINT_LEAST8_FMTu__ = "hhu";
pub const __UINT_LEAST8_FMTx__ = "hhx";
pub const __UINT_LEAST8_FMTX__ = "hhX";
pub const __INT_LEAST16_TYPE__ = c_short;
pub const __INT_LEAST16_MAX__ = @as(c_int, 32767);
pub const __INT_LEAST16_WIDTH__ = @as(c_int, 16);
pub const __INT_LEAST16_FMTd__ = "hd";
pub const __INT_LEAST16_FMTi__ = "hi";
pub const __UINT_LEAST16_TYPE__ = c_ushort;
pub const __UINT_LEAST16_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 65535, .decimal);
pub const __UINT_LEAST16_FMTo__ = "ho";
pub const __UINT_LEAST16_FMTu__ = "hu";
pub const __UINT_LEAST16_FMTx__ = "hx";
pub const __UINT_LEAST16_FMTX__ = "hX";
pub const __INT_LEAST32_TYPE__ = c_int;
pub const __INT_LEAST32_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 2147483647, .decimal);
pub const __INT_LEAST32_WIDTH__ = @as(c_int, 32);
pub const __INT_LEAST32_FMTd__ = "d";
pub const __INT_LEAST32_FMTi__ = "i";
pub const __UINT_LEAST32_TYPE__ = c_uint;
pub const __UINT_LEAST32_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_uint, 4294967295, .decimal);
pub const __UINT_LEAST32_FMTo__ = "o";
pub const __UINT_LEAST32_FMTu__ = "u";
pub const __UINT_LEAST32_FMTx__ = "x";
pub const __UINT_LEAST32_FMTX__ = "X";
pub const __INT_LEAST64_TYPE__ = c_long;
pub const __INT_LEAST64_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_long, 9223372036854775807, .decimal);
pub const __INT_LEAST64_WIDTH__ = @as(c_int, 64);
pub const __INT_LEAST64_FMTd__ = "ld";
pub const __INT_LEAST64_FMTi__ = "li";
pub const __UINT_LEAST64_TYPE__ = c_ulong;
pub const __UINT_LEAST64_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_ulong, 18446744073709551615, .decimal);
pub const __UINT_LEAST64_FMTo__ = "lo";
pub const __UINT_LEAST64_FMTu__ = "lu";
pub const __UINT_LEAST64_FMTx__ = "lx";
pub const __UINT_LEAST64_FMTX__ = "lX";
pub const __INT_FAST8_TYPE__ = i8;
pub const __INT_FAST8_MAX__ = @as(c_int, 127);
pub const __INT_FAST8_WIDTH__ = @as(c_int, 8);
pub const __INT_FAST8_FMTd__ = "hhd";
pub const __INT_FAST8_FMTi__ = "hhi";
pub const __UINT_FAST8_TYPE__ = u8;
pub const __UINT_FAST8_MAX__ = @as(c_int, 255);
pub const __UINT_FAST8_FMTo__ = "hho";
pub const __UINT_FAST8_FMTu__ = "hhu";
pub const __UINT_FAST8_FMTx__ = "hhx";
pub const __UINT_FAST8_FMTX__ = "hhX";
pub const __INT_FAST16_TYPE__ = c_short;
pub const __INT_FAST16_MAX__ = @as(c_int, 32767);
pub const __INT_FAST16_WIDTH__ = @as(c_int, 16);
pub const __INT_FAST16_FMTd__ = "hd";
pub const __INT_FAST16_FMTi__ = "hi";
pub const __UINT_FAST16_TYPE__ = c_ushort;
pub const __UINT_FAST16_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 65535, .decimal);
pub const __UINT_FAST16_FMTo__ = "ho";
pub const __UINT_FAST16_FMTu__ = "hu";
pub const __UINT_FAST16_FMTx__ = "hx";
pub const __UINT_FAST16_FMTX__ = "hX";
pub const __INT_FAST32_TYPE__ = c_int;
pub const __INT_FAST32_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_int, 2147483647, .decimal);
pub const __INT_FAST32_WIDTH__ = @as(c_int, 32);
pub const __INT_FAST32_FMTd__ = "d";
pub const __INT_FAST32_FMTi__ = "i";
pub const __UINT_FAST32_TYPE__ = c_uint;
pub const __UINT_FAST32_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_uint, 4294967295, .decimal);
pub const __UINT_FAST32_FMTo__ = "o";
pub const __UINT_FAST32_FMTu__ = "u";
pub const __UINT_FAST32_FMTx__ = "x";
pub const __UINT_FAST32_FMTX__ = "X";
pub const __INT_FAST64_TYPE__ = c_long;
pub const __INT_FAST64_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_long, 9223372036854775807, .decimal);
pub const __INT_FAST64_WIDTH__ = @as(c_int, 64);
pub const __INT_FAST64_FMTd__ = "ld";
pub const __INT_FAST64_FMTi__ = "li";
pub const __UINT_FAST64_TYPE__ = c_ulong;
pub const __UINT_FAST64_MAX__ = @import("std").zig.c_translation.promoteIntLiteral(c_ulong, 18446744073709551615, .decimal);
pub const __UINT_FAST64_FMTo__ = "lo";
pub const __UINT_FAST64_FMTu__ = "lu";
pub const __UINT_FAST64_FMTx__ = "lx";
pub const __UINT_FAST64_FMTX__ = "lX";
pub const __USER_LABEL_PREFIX__ = "";
pub const __FINITE_MATH_ONLY__ = @as(c_int, 0);
pub const __GNUC_STDC_INLINE__ = @as(c_int, 1);
pub const __GCC_ATOMIC_TEST_AND_SET_TRUEVAL = @as(c_int, 1);
pub const __CLANG_ATOMIC_BOOL_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_CHAR_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_CHAR16_T_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_CHAR32_T_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_WCHAR_T_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_SHORT_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_INT_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_LONG_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_LLONG_LOCK_FREE = @as(c_int, 2);
pub const __CLANG_ATOMIC_POINTER_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_BOOL_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_CHAR_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_CHAR16_T_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_CHAR32_T_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_WCHAR_T_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_SHORT_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_INT_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_LONG_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_LLONG_LOCK_FREE = @as(c_int, 2);
pub const __GCC_ATOMIC_POINTER_LOCK_FREE = @as(c_int, 2);
pub const __NO_INLINE__ = @as(c_int, 1);
pub const __FLT_RADIX__ = @as(c_int, 2);
pub const __DECIMAL_DIG__ = __LDBL_DECIMAL_DIG__;
pub const __AARCH64EL__ = @as(c_int, 1);
pub const __aarch64__ = @as(c_int, 1);
pub const __ELF__ = @as(c_int, 1);
pub const __AARCH64_CMODEL_SMALL__ = @as(c_int, 1);
pub const __ARM_ACLE = @as(c_int, 200);
pub const __ARM_ARCH = @as(c_int, 8);
pub const __ARM_ARCH_PROFILE = 'A';
pub const __ARM_64BIT_STATE = @as(c_int, 1);
pub const __ARM_PCS_AAPCS64 = @as(c_int, 1);
pub const __ARM_ARCH_ISA_A64 = @as(c_int, 1);
pub const __ARM_FEATURE_CLZ = @as(c_int, 1);
pub const __ARM_FEATURE_FMA = @as(c_int, 1);
pub const __ARM_FEATURE_LDREX = @as(c_int, 0xF);
pub const __ARM_FEATURE_IDIV = @as(c_int, 1);
pub const __ARM_FEATURE_DIV = @as(c_int, 1);
pub const __ARM_FEATURE_NUMERIC_MAXMIN = @as(c_int, 1);
pub const __ARM_FEATURE_DIRECTED_ROUNDING = @as(c_int, 1);
pub const __ARM_ALIGN_MAX_STACK_PWR = @as(c_int, 4);
pub const __ARM_FP = @as(c_int, 0xE);
pub const __ARM_FP16_FORMAT_IEEE = @as(c_int, 1);
pub const __ARM_FP16_ARGS = @as(c_int, 1);
pub const __ARM_SIZEOF_WCHAR_T = @as(c_int, 4);
pub const __ARM_SIZEOF_MINIMAL_ENUM = @as(c_int, 4);
pub const __ARM_NEON = @as(c_int, 1);
pub const __ARM_NEON_FP = @as(c_int, 0xE);
pub const __ARM_FEATURE_UNALIGNED = @as(c_int, 1);
pub const __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 = @as(c_int, 1);
pub const __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 = @as(c_int, 1);
pub const __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 = @as(c_int, 1);
pub const __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 = @as(c_int, 1);
pub const __FP_FAST_FMA = @as(c_int, 1);
pub const __FP_FAST_FMAF = @as(c_int, 1);
pub const __STDC__ = @as(c_int, 1);
pub const __STDC_HOSTED__ = @as(c_int, 0);
pub const __STDC_VERSION__ = @as(c_long, 201710);
pub const __STDC_UTF_16__ = @as(c_int, 1);
pub const __STDC_UTF_32__ = @as(c_int, 1);
pub const _DEBUG = @as(c_int, 1);
pub const __GCC_HAVE_DWARF2_CFI_ASM = @as(c_int, 1);
pub const __STDBOOL_H = "";
pub const __bool_true_false_are_defined = @as(c_int, 1);
pub const @"bool" = bool;
pub const @"true" = @as(c_int, 1);
pub const @"false" = @as(c_int, 0);
pub const __CLANG_STDINT_H = "";
pub const __int_least64_t = i64;
pub const __uint_least64_t = u64;
pub const __int_least32_t = i64;
pub const __uint_least32_t = u64;
pub const __int_least16_t = i64;
pub const __uint_least16_t = u64;
pub const __int_least8_t = i64;
pub const __uint_least8_t = u64;
pub const __uint32_t_defined = "";
pub const __int8_t_defined = "";
pub const __intptr_t_defined = "";
pub const _INTPTR_T = "";
pub const _UINTPTR_T = "";
pub inline fn __int_c(v: anytype, suffix: anytype) @TypeOf(__int_c_join(v, suffix)) {
    return __int_c_join(v, suffix);
}
pub const __int64_c_suffix = __INT64_C_SUFFIX__;
pub const __int32_c_suffix = __INT64_C_SUFFIX__;
pub const __int16_c_suffix = __INT64_C_SUFFIX__;
pub const __int8_c_suffix = __INT64_C_SUFFIX__;
pub inline fn INT64_C(v: anytype) @TypeOf(__int_c(v, __int64_c_suffix)) {
    return __int_c(v, __int64_c_suffix);
}
pub inline fn UINT64_C(v: anytype) @TypeOf(__uint_c(v, __int64_c_suffix)) {
    return __uint_c(v, __int64_c_suffix);
}
pub inline fn INT32_C(v: anytype) @TypeOf(__int_c(v, __int32_c_suffix)) {
    return __int_c(v, __int32_c_suffix);
}
pub inline fn UINT32_C(v: anytype) @TypeOf(__uint_c(v, __int32_c_suffix)) {
    return __uint_c(v, __int32_c_suffix);
}
pub inline fn INT16_C(v: anytype) @TypeOf(__int_c(v, __int16_c_suffix)) {
    return __int_c(v, __int16_c_suffix);
}
pub inline fn UINT16_C(v: anytype) @TypeOf(__uint_c(v, __int16_c_suffix)) {
    return __uint_c(v, __int16_c_suffix);
}
pub inline fn INT8_C(v: anytype) @TypeOf(__int_c(v, __int8_c_suffix)) {
    return __int_c(v, __int8_c_suffix);
}
pub inline fn UINT8_C(v: anytype) @TypeOf(__uint_c(v, __int8_c_suffix)) {
    return __uint_c(v, __int8_c_suffix);
}
pub const INT64_MAX = INT64_C(@import("std").zig.c_translation.promoteIntLiteral(c_int, 9223372036854775807, .decimal));
pub const INT64_MIN = -INT64_C(@import("std").zig.c_translation.promoteIntLiteral(c_int, 9223372036854775807, .decimal)) - @as(c_int, 1);
pub const UINT64_MAX = UINT64_C(@import("std").zig.c_translation.promoteIntLiteral(c_int, 18446744073709551615, .decimal));
pub const __INT_LEAST64_MIN = INT64_MIN;
pub const __INT_LEAST64_MAX = INT64_MAX;
pub const __UINT_LEAST64_MAX = UINT64_MAX;
pub const __INT_LEAST32_MIN = INT64_MIN;
pub const __INT_LEAST32_MAX = INT64_MAX;
pub const __UINT_LEAST32_MAX = UINT64_MAX;
pub const __INT_LEAST16_MIN = INT64_MIN;
pub const __INT_LEAST16_MAX = INT64_MAX;
pub const __UINT_LEAST16_MAX = UINT64_MAX;
pub const __INT_LEAST8_MIN = INT64_MIN;
pub const __INT_LEAST8_MAX = INT64_MAX;
pub const __UINT_LEAST8_MAX = UINT64_MAX;
pub const INT_LEAST64_MIN = __INT_LEAST64_MIN;
pub const INT_LEAST64_MAX = __INT_LEAST64_MAX;
pub const UINT_LEAST64_MAX = __UINT_LEAST64_MAX;
pub const INT_FAST64_MIN = __INT_LEAST64_MIN;
pub const INT_FAST64_MAX = __INT_LEAST64_MAX;
pub const UINT_FAST64_MAX = __UINT_LEAST64_MAX;
pub const INT32_MAX = INT32_C(@import("std").zig.c_translation.promoteIntLiteral(c_int, 2147483647, .decimal));
pub const INT32_MIN = -INT32_C(@import("std").zig.c_translation.promoteIntLiteral(c_int, 2147483647, .decimal)) - @as(c_int, 1);
pub const UINT32_MAX = UINT32_C(@import("std").zig.c_translation.promoteIntLiteral(c_int, 4294967295, .decimal));
pub const INT_LEAST32_MIN = __INT_LEAST32_MIN;
pub const INT_LEAST32_MAX = __INT_LEAST32_MAX;
pub const UINT_LEAST32_MAX = __UINT_LEAST32_MAX;
pub const INT_FAST32_MIN = __INT_LEAST32_MIN;
pub const INT_FAST32_MAX = __INT_LEAST32_MAX;
pub const UINT_FAST32_MAX = __UINT_LEAST32_MAX;
pub const INT16_MAX = INT16_C(@as(c_int, 32767));
pub const INT16_MIN = -INT16_C(@as(c_int, 32767)) - @as(c_int, 1);
pub const UINT16_MAX = UINT16_C(@import("std").zig.c_translation.promoteIntLiteral(c_int, 65535, .decimal));
pub const INT_LEAST16_MIN = __INT_LEAST16_MIN;
pub const INT_LEAST16_MAX = __INT_LEAST16_MAX;
pub const UINT_LEAST16_MAX = __UINT_LEAST16_MAX;
pub const INT_FAST16_MIN = __INT_LEAST16_MIN;
pub const INT_FAST16_MAX = __INT_LEAST16_MAX;
pub const UINT_FAST16_MAX = __UINT_LEAST16_MAX;
pub const INT8_MAX = INT8_C(@as(c_int, 127));
pub const INT8_MIN = -INT8_C(@as(c_int, 127)) - @as(c_int, 1);
pub const UINT8_MAX = UINT8_C(@as(c_int, 255));
pub const INT_LEAST8_MIN = __INT_LEAST8_MIN;
pub const INT_LEAST8_MAX = __INT_LEAST8_MAX;
pub const UINT_LEAST8_MAX = __UINT_LEAST8_MAX;
pub const INT_FAST8_MIN = __INT_LEAST8_MIN;
pub const INT_FAST8_MAX = __INT_LEAST8_MAX;
pub const UINT_FAST8_MAX = __UINT_LEAST8_MAX;
pub const INTPTR_MIN = -__INTPTR_MAX__ - @as(c_int, 1);
pub const INTPTR_MAX = __INTPTR_MAX__;
pub const UINTPTR_MAX = __UINTPTR_MAX__;
pub const PTRDIFF_MIN = -__PTRDIFF_MAX__ - @as(c_int, 1);
pub const PTRDIFF_MAX = __PTRDIFF_MAX__;
pub const SIZE_MAX = __SIZE_MAX__;
pub const INTMAX_MIN = -__INTMAX_MAX__ - @as(c_int, 1);
pub const INTMAX_MAX = __INTMAX_MAX__;
pub const UINTMAX_MAX = __UINTMAX_MAX__;
pub const SIG_ATOMIC_MIN = __INTN_MIN(__SIG_ATOMIC_WIDTH__);
pub const SIG_ATOMIC_MAX = __INTN_MAX(__SIG_ATOMIC_WIDTH__);
pub const WINT_MIN = __INTN_MIN(__WINT_WIDTH__);
pub const WINT_MAX = __INTN_MAX(__WINT_WIDTH__);
pub const WCHAR_MAX = __WCHAR_MAX__;
pub const WCHAR_MIN = __UINTN_C(__WCHAR_WIDTH__, @as(c_int, 0));
pub inline fn INTMAX_C(v: anytype) @TypeOf(__int_c(v, __INTMAX_C_SUFFIX__)) {
    return __int_c(v, __INTMAX_C_SUFFIX__);
}
pub inline fn UINTMAX_C(v: anytype) @TypeOf(__int_c(v, __UINTMAX_C_SUFFIX__)) {
    return __int_c(v, __UINTMAX_C_SUFFIX__);
}
pub const __thread = "";
pub const CONFIG_ARM_HIKEY_OUTSTANDING_PREFETCHERS = @as(c_int, 0);
pub const CONFIG_ARM_HIKEY_PREFETCHER_STRIDE = @as(c_int, 0);
pub const CONFIG_ARM_HIKEY_PREFETCHER_NPFSTRM = @as(c_int, 0);
pub const CONFIG_ARCH_AARCH64 = @as(c_int, 1);
pub const CONFIG_ARCH_ARM = @as(c_int, 1);
pub const CONFIG_WORD_SIZE = @as(c_int, 64);
pub const CONFIG_USER_TOP = @import("std").zig.c_translation.promoteIntLiteral(c_int, 0xa0000000, .hexadecimal);
pub const CONFIG_PLAT_QEMU_ARM_VIRT = @as(c_int, 1);
pub const CONFIG_ARM_CORTEX_A53 = @as(c_int, 1);
pub const CONFIG_ARCH_ARM_V8A = @as(c_int, 1);
pub const CONFIG_ARM_MACH = "";
pub const CONFIG_KERNEL_MCS = @as(c_int, 1);
pub const CONFIG_ARM_PA_SIZE_BITS_40 = @as(c_int, 1);
pub const CONFIG_ARM_ICACHE_VIPT = @as(c_int, 1);
pub const CONFIG_ARM_HYPERVISOR_SUPPORT = @as(c_int, 1);
pub const CONFIG_AARCH64_USER_CACHE_ENABLE = @as(c_int, 1);
pub const CONFIG_L1_CACHE_LINE_SIZE_BITS = @as(c_int, 6);
pub const CONFIG_EXPORT_PCNT_USER = @as(c_int, 1);
pub const CONFIG_VTIMER_UPDATE_VOFFSET = @as(c_int, 1);
pub const CONFIG_HAVE_FPU = @as(c_int, 1);
pub const CONFIG_PADDR_USER_DEVICE_TOP = @import("std").zig.c_translation.promoteIntLiteral(c_int, 1099511627776, .decimal);
pub const CONFIG_ROOT_CNODE_SIZE_BITS = @as(c_int, 12);
pub const CONFIG_BOOT_THREAD_TIME_SLICE = @as(c_int, 5);
pub const CONFIG_RETYPE_FAN_OUT_LIMIT = @as(c_int, 256);
pub const CONFIG_MAX_NUM_WORK_UNITS_PER_PREEMPTION = @as(c_int, 100);
pub const CONFIG_RESET_CHUNK_BITS = @as(c_int, 8);
pub const CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS = @as(c_int, 230);
pub const CONFIG_FASTPATH = @as(c_int, 1);
pub const CONFIG_NUM_DOMAINS = @as(c_int, 1);
pub const CONFIG_NUM_PRIORITIES = @as(c_int, 256);
pub const CONFIG_MAX_NUM_NODES = @as(c_int, 1);
pub const CONFIG_KERNEL_STACK_BITS = @as(c_int, 12);
pub const CONFIG_FPU_MAX_RESTORES_SINCE_SWITCH = @as(c_int, 64);
pub const CONFIG_DEBUG_BUILD = @as(c_int, 1);
pub const CONFIG_PRINTING = @as(c_int, 1);
pub const CONFIG_NO_BENCHMARKS = @as(c_int, 1);
pub const CONFIG_MAX_NUM_TRACE_POINTS = @as(c_int, 0);
pub const CONFIG_IRQ_REPORTING = @as(c_int, 1);
pub const CONFIG_COLOUR_PRINTING = @as(c_int, 1);
pub const CONFIG_USER_STACK_TRACE_LENGTH = @as(c_int, 16);
pub const CONFIG_KERNEL_OPT_LEVEL_O2 = @as(c_int, 1);
pub const CONFIG_KERNEL_OPTIMISATION_CLONE_FUNCTIONS = @as(c_int, 1);
pub const CONFIG_KERNEL_WCET_SCALE = @as(c_int, 1);
pub const CONFIG_KERNEL_STATIC_MAX_PERIOD_US = @as(c_int, 0);
pub const CONFIG_LIB_SEL4_INLINE_INVOCATIONS = @as(c_int, 1);
pub const CONFIG_LIB_SEL4_PRINT_INVOCATION_ERRORS = @as(c_int, 0);
pub inline fn LIBSEL4_BIT(n: anytype) @TypeOf(@as(c_ulong, 1) << n) {
    return @as(c_ulong, 1) << n;
}
pub const SEL4_WORD_IS_UINT64 = "";
pub const SEL4_INT64_IS_LONG = "";
pub const _seL4_int8_type = u8;
pub const _seL4_int16_type = c_short;
pub const _seL4_int32_type = c_int;
pub const _seL4_int64_type = c_long;
pub inline fn _macro_str_concat(x: anytype, y: anytype) @TypeOf(_macro_str_concat_helper1(x, y)) {
    return _macro_str_concat_helper1(x, y);
}
pub const seL4_True = @as(c_int, 1);
pub const seL4_False = @as(c_int, 0);
pub const seL4_Null = @import("std").zig.c_translation.cast(?*anyopaque, @as(c_int, 0));
pub const _seL4_word_fmt = _seL4_int64_fmt;
pub const SEL4_PRI_word = SEL4_PRIu_word;
pub const seL4_DataFault = @as(c_int, 0);
pub const seL4_InstructionFault = @as(c_int, 1);
pub const seL4_PageBits = @as(c_int, 12);
pub const seL4_LargePageBits = @as(c_int, 21);
pub const seL4_HugePageBits = @as(c_int, 30);
pub const seL4_SlotBits = @as(c_int, 5);
pub const seL4_TCBBits = @as(c_int, 11);
pub const seL4_EndpointBits = @as(c_int, 4);
pub const seL4_NotificationBits = @as(c_int, 6);
pub const seL4_ReplyBits = @as(c_int, 5);
pub const seL4_PageTableBits = @as(c_int, 12);
pub const seL4_PageTableEntryBits = @as(c_int, 3);
pub const seL4_PageTableIndexBits = @as(c_int, 9);
pub const seL4_PageDirBits = @as(c_int, 12);
pub const seL4_PageDirEntryBits = @as(c_int, 3);
pub const seL4_PageDirIndexBits = @as(c_int, 9);
pub const seL4_NumASIDPoolsBits = @as(c_int, 7);
pub const seL4_ASIDPoolBits = @as(c_int, 12);
pub const seL4_ASIDPoolIndexBits = @as(c_int, 9);
pub const seL4_IOPageTableBits = @as(c_int, 12);
pub const seL4_WordSizeBits = @as(c_int, 3);
pub const seL4_PUDEntryBits = @as(c_int, 3);
pub const seL4_PGDBits = @as(c_int, 0);
pub const seL4_PGDEntryBits = @as(c_int, 0);
pub const seL4_PGDIndexBits = @as(c_int, 0);
pub const seL4_PUDBits = @as(c_int, 13);
pub const seL4_PUDIndexBits = @as(c_int, 10);
pub const seL4_VSpaceBits = seL4_PUDBits;
pub const seL4_VSpaceIndexBits = seL4_PUDIndexBits;
pub const seL4_ARM_VSpaceObject = seL4_ARM_PageUpperDirectoryObject;
pub const seL4_ARM_VCPUBits = @as(c_int, 12);
pub const seL4_VCPUBits = @as(c_int, 12);
pub const seL4_WordBits = @import("std").zig.c_translation.sizeof(seL4_Word) * @as(c_int, 8);
pub const seL4_MinUntypedBits = @as(c_int, 4);
pub const seL4_MaxUntypedBits = @as(c_int, 47);
pub const seL4_FastMessageRegisters = @as(c_int, 4);
pub const seL4_IPCBufferSizeBits = @as(c_int, 10);
pub const seL4_UserTop = @import("std").zig.c_translation.promoteIntLiteral(c_int, 0x000000ffffffffff, .hexadecimal);
pub inline fn seL4_CompileTimeAssert(expr: anytype) @TypeOf(SEL4_COMPILE_ASSERT(seL4_CompileTimeAssert, expr)) {
    return SEL4_COMPILE_ASSERT(seL4_CompileTimeAssert, expr);
}
pub inline fn seL4_DebugAssert(expr: anytype) @TypeOf(seL4_Assert(expr)) {
    return seL4_Assert(expr);
}
pub const seL4_ARM_IOPageTableObject = @import("std").zig.c_translation.promoteIntLiteral(c_int, 0xffff, .hexadecimal);
pub const seL4_MsgMaxExtraCaps = LIBSEL4_BIT(seL4_MsgExtraCapBits) - @as(c_int, 1);
pub const seL4_CapRightsBits = @as(c_int, 4);
pub const seL4_MinSchedContextBits = @as(c_int, 7);
pub const seL4_CoreSchedContextBytes = (@as(c_int, 10) * @import("std").zig.c_translation.sizeof(seL4_Word)) + (@as(c_int, 6) * @as(c_int, 8));
pub const seL4_RefillSizeBytes = @as(c_int, 2) * @as(c_int, 8);
pub const seL4_ReadWrite = seL4_CapRights_new(@as(c_int, 0), @as(c_int, 0), @as(c_int, 1), @as(c_int, 1));
pub const seL4_AllRights = seL4_CapRights_new(@as(c_int, 1), @as(c_int, 1), @as(c_int, 1), @as(c_int, 1));
pub const seL4_CanRead = seL4_CapRights_new(@as(c_int, 0), @as(c_int, 0), @as(c_int, 1), @as(c_int, 0));
pub const seL4_CanWrite = seL4_CapRights_new(@as(c_int, 0), @as(c_int, 0), @as(c_int, 0), @as(c_int, 1));
pub const seL4_CanGrant = seL4_CapRights_new(@as(c_int, 0), @as(c_int, 1), @as(c_int, 0), @as(c_int, 0));
pub const seL4_CanGrantReply = seL4_CapRights_new(@as(c_int, 1), @as(c_int, 0), @as(c_int, 0), @as(c_int, 0));
pub const seL4_NoWrite = seL4_CapRights_new(@as(c_int, 1), @as(c_int, 1), @as(c_int, 1), @as(c_int, 0));
pub const seL4_NoRead = seL4_CapRights_new(@as(c_int, 1), @as(c_int, 1), @as(c_int, 0), @as(c_int, 1));
pub const seL4_NoRights = seL4_CapRights_new(@as(c_int, 0), @as(c_int, 0), @as(c_int, 0), @as(c_int, 0));
pub const seL4_GuardSizeBits = @as(c_int, 6);
pub const seL4_GuardBits = @as(c_int, 58);
pub const seL4_BadgeBits = @as(c_int, 64);
pub const seL4_UntypedRetypeMaxObjects = CONFIG_RETYPE_FAN_OUT_LIMIT;
pub const seL4_NilData = @as(c_int, 0);
pub const seL4_CapInitThreadPD = seL4_CapInitThreadVSpace;
pub const SEL4_PFIPC_LABEL = SEL4_DEPRECATE_MACRO(seL4_Fault_VMFault);
pub const SEL4_PFIPC_LENGTH = SEL4_DEPRECATE_MACRO(seL4_VMFault_Length);
pub const SEL4_PFIPC_FAULT_IP = SEL4_DEPRECATE_MACRO(seL4_VMFault_IP);
pub const SEL4_PFIPC_FAULT_ADDR = SEL4_DEPRECATE_MACRO(seL4_VMFault_Addr);
pub const SEL4_PFIPC_PREFETCH_FAULT = SEL4_DEPRECATE_MACRO(seL4_VMFault_PrefetchFault);
pub const SEL4_PFIPC_FSR = SEL4_DEPRECATE_MACRO(seL4_VMFault_FSR);
pub const SEL4_EXCEPT_IPC_LABEL = SEL4_DEPRECATE_MACRO(seL4_Fault_UnknownSyscall);
pub const SEL4_USER_EXCEPTION_LABEL = SEL4_DEPRECATE_MACRO(seL4_Fault_UserException);
pub const SEL4_USER_EXCEPTION_LENGTH = SEL4_DEPRECATE_MACRO(seL4_UserException_Length);
pub const SEL4_VGIC_MAINTENANCE_LENGTH = SEL4_DEPRECATE_MACRO(seL4_VGICMaintenance_Length);
pub const SEL4_VGIC_MAINTENANCE_LABEL = SEL4_DEPRECATE_MACRO(seL4_Fault_VGICMaintenance);
pub const SEL4_VCPU_FAULT_LENGTH = SEL4_DEPRECATE_MACRO(seL4_VCPUFault_Length);
pub const SEL4_VCPU_FAULT_LABEL = SEL4_DEPRECATE_MACRO(seL4_Fault_VCPUFault);
pub const seL4_NoFault = SEL4_DEPRECATE_MACRO(seL4_Fault_NullFault);
pub const seL4_CapFault = SEL4_DEPRECATE_MACRO(seL4_Fault_CapFault);
pub const seL4_UnknownSyscall = SEL4_DEPRECATE_MACRO(seL4_Fault_UnknownSyscall);
pub const seL4_UserException = SEL4_DEPRECATE_MACRO(seL4_Fault_UserException);
pub const seL4_VMFault = SEL4_DEPRECATE_MACRO(seL4_Fault_VMFault);
pub const seL4_NumHWBreakpoints = @as(c_int, 10);
pub const seL4_NumExclusiveBreakpoints = @as(c_int, 6);
pub const seL4_NumExclusiveWatchpoints = @as(c_int, 4);
pub const MONITOR_ENDPOINT_CAP = @as(c_int, 5);
pub const TCB_CAP = @as(c_int, 6);
pub const BASE_OUTPUT_NOTIFICATION_CAP = @as(c_int, 10);
pub const BASE_ENDPOINT_CAP = @as(c_int, 74);
pub const BASE_IRQ_CAP = @as(c_int, 138);
pub const BASE_TCB_CAP = @as(c_int, 202);
pub const BASE_VM_TCB_CAP = @as(c_int, 266);
pub const BASE_VCPU_CAP = @as(c_int, 330);
pub const SEL4CP_MAX_CHANNELS = @as(c_int, 63);
pub const seL4_UserContext_ = struct_seL4_UserContext_;
pub const seL4_Fault = struct_seL4_Fault;
pub const seL4_Fault_tag = enum_seL4_Fault_tag;
pub const api_object = enum_api_object;
pub const _mode_object = enum__mode_object;
pub const _object = enum__object;
pub const priorityConstants = enum_priorityConstants;
pub const seL4_MsgLimits = enum_seL4_MsgLimits;
pub const seL4_CNode_CapData = struct_seL4_CNode_CapData;
pub const seL4_MessageInfo = struct_seL4_MessageInfo;
pub const seL4_IPCBuffer_ = struct_seL4_IPCBuffer_;
pub const invocation_label = enum_invocation_label;
pub const sel4_arch_invocation_label = enum_sel4_arch_invocation_label;
pub const arch_invocation_label = enum_arch_invocation_label;
pub const seL4_ARM_SIDControl_GetFault = struct_seL4_ARM_SIDControl_GetFault;
pub const seL4_ARM_CB_CBGetFault = struct_seL4_ARM_CB_CBGetFault;
pub const seL4_TCB_GetBreakpoint = struct_seL4_TCB_GetBreakpoint;
pub const seL4_TCB_ConfigureSingleStepping = struct_seL4_TCB_ConfigureSingleStepping;
