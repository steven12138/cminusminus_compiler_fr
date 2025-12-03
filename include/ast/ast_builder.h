//
// Created by steven on 12/3/25.
//
#pragma once
#include "ast.h"

namespace front::ast {
    // --- General / Forwarding ---
    // type1. copy/forward single child
    SemVal build_single_forward(std::vector<SemVal> &rhs);

    // --- Types ---
    // type2. Btype/FuncType -> BasicType
    SemVal build_type_int(std::vector<SemVal> &rhs);

    SemVal build_type_float(std::vector<SemVal> &rhs);

    SemVal build_type_void(std::vector<SemVal> &rhs);

    // --- Program Structure ---
    // CompUnitList building
    SemVal build_comp_unit_list_item(std::vector<SemVal> &rhs);

    SemVal build_comp_unit_list_append(std::vector<SemVal> &rhs);

    // --- Declarations ---
    // Decl builders
    SemVal build_const_decl(std::vector<SemVal> &rhs);

    SemVal build_var_decl(std::vector<SemVal> &rhs);

    // Def lists (ConstDefList, VarDefList) -> std::vector<VarInit>
    SemVal build_def_list_item(std::vector<SemVal> &rhs);

    SemVal build_def_list_append(std::vector<SemVal> &rhs);

    // Individual Definitions -> std::vector<VarInit> (size 1) to match list type
    SemVal build_const_def(std::vector<SemVal> &rhs);

    SemVal build_var_def_uninit(std::vector<SemVal> &rhs);

    SemVal build_var_def_init(std::vector<SemVal> &rhs);

    // --- Functions ---
    SemVal build_func_def(std::vector<SemVal> &rhs);

    SemVal build_func_def_no_params(std::vector<SemVal> &rhs); // Handle case without params if grammar splits it

    // FuncParams -> std::vector<Param>
    SemVal build_func_fparams_item(std::vector<SemVal> &rhs);

    SemVal build_func_fparams_append(std::vector<SemVal> &rhs);

    SemVal build_func_fparam(std::vector<SemVal> &rhs);

    // --- Block / Statements ---
    SemVal build_block(std::vector<SemVal> &rhs);

    SemVal build_block_empty(std::vector<SemVal> &rhs); // {}

    // BlockItems -> std::vector<BlockItem>
    SemVal build_block_item_list_item(std::vector<SemVal> &rhs);

    SemVal build_block_item_list_append(std::vector<SemVal> &rhs);

    SemVal build_block_item_decl(std::vector<SemVal> &rhs);

    SemVal build_block_item_stmt(std::vector<SemVal> &rhs);

    // Statements
    SemVal build_stmt_assign(std::vector<SemVal> &rhs);

    SemVal build_stmt_exp(std::vector<SemVal> &rhs);

    SemVal build_stmt_empty(std::vector<SemVal> &rhs);

    SemVal build_stmt_if(std::vector<SemVal> &rhs);

    SemVal build_stmt_if_else(std::vector<SemVal> &rhs);

    SemVal build_stmt_return(std::vector<SemVal> &rhs);

    SemVal build_stmt_return_void(std::vector<SemVal> &rhs);

    // --- Expressions ---
    // Basic literals
    SemVal build_exp_int(std::vector<SemVal> &rhs);

    SemVal build_exp_float(std::vector<SemVal> &rhs);

    // LVal handling
    SemVal build_lval_ident(std::vector<SemVal> &rhs); // Returns string
    SemVal build_exp_lval(std::vector<SemVal> &rhs); // Returns ExprPtr(IdentifierExpr)

    // Function Calls
    SemVal build_exp_call(std::vector<SemVal> &rhs);

    SemVal build_exp_call_void(std::vector<SemVal> &rhs);

    // Arguments (FuncRParams) - Reusing vector<VarInit> as container for ExprPtrs since SemVal lacks vector<ExprPtr>
    SemVal build_func_rparams_item(std::vector<SemVal> &rhs);

    SemVal build_func_rparams_append(std::vector<SemVal> &rhs);

    // Operators
    SemVal build_unary_op_positive(std::vector<SemVal> &rhs);

    SemVal build_unary_op_negative(std::vector<SemVal> &rhs);

    SemVal build_unary_op_not(std::vector<SemVal> &rhs);

    SemVal build_unary_exp(std::vector<SemVal> &rhs);

    // Binary Ops Helpers
    SemVal build_binary_add(std::vector<SemVal> &rhs);

    SemVal build_binary_sub(std::vector<SemVal> &rhs);

    SemVal build_binary_mul(std::vector<SemVal> &rhs);

    SemVal build_binary_div(std::vector<SemVal> &rhs);

    SemVal build_binary_mod(std::vector<SemVal> &rhs);

    SemVal build_binary_lt(std::vector<SemVal> &rhs);

    SemVal build_binary_gt(std::vector<SemVal> &rhs);

    SemVal build_binary_le(std::vector<SemVal> &rhs);

    SemVal build_binary_ge(std::vector<SemVal> &rhs);

    SemVal build_binary_eq(std::vector<SemVal> &rhs);

    SemVal build_binary_neq(std::vector<SemVal> &rhs);

    SemVal build_binary_and(std::vector<SemVal> &rhs);

    SemVal build_binary_or(std::vector<SemVal> &rhs);
}
