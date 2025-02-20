// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*- // technically C99

#define _GNU_SOURCE

#include "x86-interval-highwatermark.h"
#include "x86-decoder.h"
#include "x86-interval-arg.h"

#include "../../utilities/arch/x86-family/instruction-set.h"

int
x86_bp_size(xed_reg_enum_t reg)
{
  switch(reg) {
#if defined (HOST_CPU_x86_64)
  case XED_REG_RBP: return 8;
#endif
  case XED_REG_EBP: return 4;
  case XED_REG_BP: return 2;
  default: return 1;
  }
}


unwind_interval *
process_move(xed_decoded_inst_t *xptr, const xed_inst_t *xi, interval_arg_t *iarg)
{
  unwind_interval *next = iarg->current;
  highwatermark_t *hw_tmp = &(iarg->highwatermark);

  const xed_operand_t *op0 =  xed_inst_operand(xi, 0);
  const xed_operand_t *op1 =  xed_inst_operand(xi, 1);

  xed_operand_enum_t   op0_name = xed_operand_name(op0);
  xed_operand_enum_t   op1_name = xed_operand_name(op1);
  x86recipe_t *xr = UWI_RECIPE(iarg->current);
  x86registers_t reg = xr->reg;

  if ((op0_name == XED_OPERAND_MEM0) && (op1_name == XED_OPERAND_REG0)) {
    //------------------------------------------------------------------------
    // storing a register to memory
    //------------------------------------------------------------------------
    xed_reg_enum_t basereg = xed_decoded_inst_get_base_reg(xptr, 0);
    if (x86_isReg_SP(basereg)) {
      //----------------------------------------------------------------------
      // a memory move with SP as a base register
      //----------------------------------------------------------------------
      xed_reg_enum_t reg1 = xed_decoded_inst_get_reg(xptr, op1_name);
      if (x86_isReg_BP(reg1) ||
        (x86_isReg_AX(reg1) && (iarg->rax_rbp_equivalent_at == iarg->ins))){
        //--------------------------------------------------------------------
        // register being stored is BP (or a copy in RAX)
        //--------------------------------------------------------------------
        if (reg.bp_status == BP_UNCHANGED) {
          //==================================================================
          // instruction: save caller's BP into the stack
          // action:      create a new interval with
          //                (1) BP status reset to BP_SAVED
          //                (2) BP position relative to the stack pointer set
          //                    to the offset from SP
          //==================================================================
          reg.bp_status = BP_SAVED;
          reg.sp_bp_pos = xed_decoded_inst_get_memory_displacement(xptr, 0);
          next = new_ui(nextInsn(iarg, xptr), xr->ra_status, &reg);
          hw_tmp->uwi = next;
          hw_tmp->state =
            HW_NEW_STATE(hw_tmp->state, HW_BP_SAVED);
        }
      }
    }
  } else if ((op1_name == XED_OPERAND_MEM0) && (op0_name == XED_OPERAND_REG0)) {
    //----------------------------------------------------------------------
    // loading a register from memory
    //----------------------------------------------------------------------
    xed_reg_enum_t reg0 = xed_decoded_inst_get_reg(xptr, op0_name);
    if (x86_isReg_BP(reg0)) {
      //--------------------------------------------------------------------
      // register being loaded is BP
      //--------------------------------------------------------------------
      if (reg.bp_status != BP_UNCHANGED) {
        int64_t offset = xed_decoded_inst_get_memory_displacement(xptr, 0);
        xed_reg_enum_t basereg = xed_decoded_inst_get_base_reg(xptr, 0);
        if (x86_isReg_SP(basereg) && (offset == reg.sp_bp_pos)) {
          //================================================================
          // instruction: restore BP from its saved location in the stack
          // action:      create a new interval with BP status reset to
          //              BP_UNCHANGED
          //================================================================
          reg.bp_status = BP_UNCHANGED;
          next = new_ui(nextInsn(iarg, xptr), RA_SP_RELATIVE, &reg);
        } else {
          //================================================================
          // instruction: BP is loaded from a memory address DIFFERENT from
          // its saved location in the stack
          // action:      create a new interval with BP status reset to
          //              BP_HOSED
          //================================================================
          if (reg.bp_status != BP_HOSED) {
            reg.bp_status = BP_HOSED;
            next = new_ui(nextInsn(iarg, xptr), RA_SP_RELATIVE, &reg);
            if (HW_TEST_STATE(hw_tmp->state, HW_BP_SAVED,
                              HW_BP_OVERWRITTEN) &&
                (UWI_RECIPE(hw_tmp->uwi)->reg.sp_ra_pos == UWI_RECIPE(next)->reg.sp_ra_pos)) {
              hw_tmp->uwi = next;
              hw_tmp->state =
                HW_NEW_STATE(hw_tmp->state, HW_BP_OVERWRITTEN);
            }
          }
        }
      }
    } else if (x86_isReg_SP(reg0)) {
      //--------------------------------------------------------------------
      // register being loaded is SP
      //--------------------------------------------------------------------
      xed_reg_enum_t basereg = xed_decoded_inst_get_base_reg(xptr, 0);
      if (x86_isReg_SP(basereg)) {
        //================================================================
        // instruction: restore SP from a saved location in the stack
        // action:      create a new interval with SP status reset to
        //              BP_UNCHANGED
        //================================================================
        reg.sp_ra_pos = 0;
        reg.bp_ra_pos = 0;
        next = new_ui(nextInsn(iarg, xptr), RA_SP_RELATIVE, &reg);
      }
    }
  } else if ((op0_name == XED_OPERAND_REG0) && (op1_name == XED_OPERAND_REG1)){
    //----------------------------------------------------------------------
    // register-to-register move
    //----------------------------------------------------------------------
    xed_reg_enum_t reg0 = xed_decoded_inst_get_reg(xptr, op0_name);
    xed_reg_enum_t reg1 = xed_decoded_inst_get_reg(xptr, op1_name);
    if (x86_isReg_BP(reg1) && x86_isReg_SP(reg0)) {
      //====================================================================
      // instruction: restore SP from BP
      // action:      begin a new SP_RELATIVE interval
      //====================================================================
      reg.sp_ra_pos = reg.bp_ra_pos;
      reg.sp_bp_pos = reg.bp_bp_pos;
      next = new_ui(nextInsn(iarg, xptr), RA_SP_RELATIVE, &reg);
    } else if (x86_isReg_BP(reg0) && x86_isReg_SP(reg1)) {
      //====================================================================
      // instruction: initialize BP with value of SP to set up a frame ptr
      // action:      begin a new interval
      //====================================================================
      reg.bp_status = BP_SAVED;
      reg.bp_ra_pos = reg.sp_ra_pos;
      reg.bp_bp_pos = reg.sp_bp_pos;
      next = new_ui(nextInsn(iarg, xptr), RA_STD_FRAME, &reg);
      if (iarg->sp_realigned) {
        // SP was previously realigned. correct RA offsets based on typical
        // frame layout in these circumstances.

        // assume RA is in word below BP
        UWI_RECIPE(next)->reg.bp_ra_pos =
          (UWI_RECIPE(next)->reg.bp_bp_pos + x86_bp_size(reg0));

        // RA offset wrt SP is the same, since SP == BP
        UWI_RECIPE(next)->reg.sp_ra_pos = UWI_RECIPE(next)->reg.bp_ra_pos;

        // once we've handled SP realignment in the routine prologue, we can
        // ignore it for the rest of the routine.
        iarg->sp_realigned = false;
      }
      if (HW_TEST_STATE(hw_tmp->state, HW_BP_SAVED,
                        HW_BP_OVERWRITTEN)) {
        hw_tmp->uwi = next;
        hw_tmp->state =
          HW_NEW_STATE(hw_tmp->state, HW_BP_OVERWRITTEN);
      }
    } else if (x86_isReg_BP(reg1) && x86_isReg_AX(reg0)) {
      //====================================================================
      // instruction: copy BP to RAX
      //====================================================================
      iarg->rax_rbp_equivalent_at = nextInsn(iarg, xptr);
    } else if (x86_isReg_BP(reg0)) {
      if (reg.bp_status != BP_HOSED){
        //==================================================================
        // instruction: move some NON-special register to BP
        // state:       bp_status is NOT BP_HOSED
        // action:      begin a new RA_SP_RELATIVE,BP_HOSED interval
        //==================================================================
        reg.bp_status = BP_HOSED;
        reg.bp_ra_pos = reg.sp_ra_pos;
        reg.bp_bp_pos = reg.sp_bp_pos;
        next = new_ui(nextInsn(iarg, xptr), RA_SP_RELATIVE, &reg);
        if (HW_TEST_STATE(hw_tmp->state, HW_BP_SAVED,
                          HW_BP_OVERWRITTEN) &&
            (UWI_RECIPE(hw_tmp->uwi)->reg.sp_ra_pos == UWI_RECIPE(next)->reg.sp_ra_pos)) {
          hw_tmp->uwi = next;
          hw_tmp->state =
            HW_NEW_STATE(hw_tmp->state, HW_BP_OVERWRITTEN);
        }
      }
    }
  }
  return next;
}
