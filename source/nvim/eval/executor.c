/// @file nvim/eval/executor.c

#include "nvim/eval/typval.h"
#include "nvim/eval/executor.h"
#include "nvim/eval.h"
#include "nvim/message.h"
#include "nvim/vim.h"
#include "nvim/globals.h"

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "eval/executor.c.generated.h"
#endif

static char *e_letwrong = N_("E734: Wrong variable type for %s=");

char *e_listidx = N_("E684: list index out of range: %" PRId64);

/// Hanle tv1 += tv2, -=, .=
///
/// @param[in,out] tv1  First operand, modified typval.
/// @param[in] tv2      Second operand.
/// @param[in] op       Used operator.
///
/// @return OK or FAIL.
int eexe_mod_op(typval_st *const tv1,
                const typval_st *const tv2,
                const char *const op)
FUNC_ATTR_NONNULL_ALL
{
    // Can't do anything with a Funcref, a Dict or special value on the right.
    if(tv2->v_type != kNvarUfunc && tv2->v_type != kNvarDict)
    {
        switch(tv1->v_type)
        {
            case kNvarDict:
            case kNvarUfunc:
            case kNvarPartial:
            case kNvarSpecial:
            {
                break;
            }

            case kNvarList:
            {
                if(*op != '+' || tv2->v_type != kNvarList)
                {
                    break;
                }

                // List += List
                if(tv1->vval.v_list != NULL && tv2->vval.v_list != NULL)
                {
                    tv_list_extend(tv1->vval.v_list, tv2->vval.v_list, NULL);
                }

                return OK;
            }

            case kNvarNumber:
            case kNvarString:
            {
                if(tv2->v_type == kNvarList)
                {
                    break;
                }

                if(*op == '+' || *op == '-')
                {
                    // nr += nr  or  nr -= nr
                    number_kt n = tv_get_number(tv1);

                    if(tv2->v_type == kNvarFloat)
                    {
                        float_kt f = n;

                        if(*op == '+')
                        {
                            f += tv2->vval.v_float;
                        }
                        else
                        {
                            f -= tv2->vval.v_float;
                        }

                        tv_clear(tv1);
                        tv1->v_type = kNvarFloat;
                        tv1->vval.v_float = f;
                    }
                    else
                    {
                        if(*op == '+')
                        {
                            n += tv_get_number(tv2);
                        }
                        else
                        {
                            n -= tv_get_number(tv2);
                        }

                        tv_clear(tv1);
                        tv1->v_type = kNvarNumber;
                        tv1->vval.v_number = n;
                    }
                }
                else
                {
                    // str .= str
                    if(tv2->v_type == kNvarFloat)
                    {
                        break;
                    }

                    const char *tvs = tv_get_string(tv1);
                    char numbuf[NUMBUFLEN];

                    char *const s =
                        (char *)concat_str((const uchar_kt *)tvs,
                                           (const uchar_kt *)tv_get_string_buf(tv2,
                                                                             numbuf));
                    tv_clear(tv1);
                    tv1->v_type = kNvarString;
                    tv1->vval.v_string = (uchar_kt *)s;
                }

                return OK;
            }

            case kNvarFloat:
            {
                bool flag = tv2->v_type != kNvarFloat
                            && tv2->v_type != kNvarNumber
                            && tv2->v_type != kNvarString;

                if(*op == '.' || flag)
                {
                    break;
                }

                const float_kt f = (tv2->v_type == kNvarFloat
                                   ? tv2->vval.v_float : tv_get_number(tv2));

                if(*op == '+')
                {
                    tv1->vval.v_float += f;
                }
                else
                {
                    tv1->vval.v_float -= f;
                }

                return OK;
            }

            case kNvarUnknown:
            {
                assert(false);
            }
        }
    }

    EMSG2(_(e_letwrong), op);

    return FAIL;
}
