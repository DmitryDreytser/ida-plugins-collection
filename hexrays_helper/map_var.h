//////////////////////////////////////////////////////////////////////////
#pragma once;

/// �������ӳ��
bool idaapi map_var_to(void *ud);

/// ���������ӳ��
bool idaapi ummap_var_from(void *ud);

/// ��ȡ��ʼ����Ϣ
void init_map_var_if();

/// �޸ı�����
void change_var_name(cfunc_t *cfunc);

// �жϵ�ǰ��괦�����Ƿ��ܷ�ӳ��
bool is_var_can_unmap(vdui_t &vu);

// ���ص�ǰҪ������ĺ����Ƿ��������ǵĲ�������
bool is_func_in_list(ea_t ea);