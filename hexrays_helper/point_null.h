#pragma once;

// ִ��ת��
void safe_convert(cfunc_t *);

// ����
void point_null_run();

// ���ع�������еı��ʽ
bool idaapi hide_this_line(void *ud);

// ��ǰ���Ƿ��������
bool is_current_line_can_hide(vdui_t &vu);

// �Զ����� var1 = var1;��ʽ�ĸ�ֵ
void hide_if_asg_equal_var(cfunc_t *cfunc);