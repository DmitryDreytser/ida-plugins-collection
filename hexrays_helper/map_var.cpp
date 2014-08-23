//////////////////////////////////////////////////////////////////////////
// 
#include <hexrays.hpp>

/// �洢����ӳ����Ϣ�Ľṹ
static const char nodename[] = "$ hexrays map_var";
static netnode var_node;
/*! \brief ���ڱ����滻�Ľṹ*/
typedef struct var_info{
	int new_name_index; ///< ������
	int old_name_index; ///< ������	
	ea_t func_addr;		///< ������ַ
	bool operator== (const var_info& vi) const
	{
		if (old_name_index == vi.old_name_index &&
			new_name_index == vi.new_name_index &&
			func_addr      == vi.func_addr) 
		{
			return true;
		}
		return false;
	}
}var_info;
static qvector<var_info> var_map_info; ///< �¾ɱ�����������

static const char res_nodename[] = "$ hexrays res_var";
static netnode res_node;
/*! \brief ��ԭ������Ҫ�õ��Ľṹ �����˾ɱ��������ʽ�������븸���ʽ�ĵ�ַ*/
typedef struct restore_info{
	int		old_idx;///< ����
	ctype_t	op;		///< �����ʽ������
	ea_t	defea;	///< �����ʽ�ĵ�ַ
	ea_t	func_addr;		///< ������ַ
	bool operator== (const restore_info& vi) const
	{
		if (old_idx == vi.old_idx && op == vi.op && defea == vi.defea && func_addr == vi.func_addr) 
		{
			return true;
		}
		return false;
	}
}restore_info;
static qvector<restore_info> res_map_info; ///< �ɱ�����ʹ����Ϣ,���ڻ�ԭ

// ����ӳ��ѡ�����п�
static const int widths[] = {10, 15};
// ����ӳ��ѡ���ı���
static const char *header[] ={"Var type", "var name"};
CASSERT(qnumber(widths) == qnumber(header));

//////////////////////////////////////////////////////////////////////////
// ����ӳ��
//////////////////////////////////////////////////////////////////////////
/*!
  \brief: ��ʼ��ʱ�����ݿ��ж�ȡ��Ϣ
  \return: void
*/
void init_map_var_if()
{
	if (!var_node.create(nodename)) 
	{
		// ����Ѵ�����ʧ��,��ȡ����
		size_t n = var_node.altval(-1);
		if ( n > 0 )
		{ // ��ȡ��Ϣ
			var_map_info.resize(n);
			n *= sizeof(var_info);
			var_node.getblob(&var_map_info[0], &n, 0, 'S');
		}
	}

	if (!res_node.create(res_nodename)) 
	{
		// ����Ѵ�����ʧ��,��ȡ����
		size_t n = res_node.altval(-1);
		if ( n > 0 )
		{ // ��ȡ��Ϣ
			res_map_info.resize(n);
			n *= sizeof(restore_info);
			res_node.getblob(&res_map_info[0], &n, 0, 'J');
		}
	}
	
	return;
}

/*!
  \brief: �޸ı�����
  \return: void
  \param cfunc
*/
void change_var_name(cfunc_t *cfunc)
{
	// �õ��������޸����б���
	struct if_inverter_t : public ctree_visitor_t
	{
		int old_idx;
		int new_idx;
		ea_t func_addr;
		if_inverter_t(int idx_o, int idx_n, ea_t addr) : 
		ctree_visitor_t(CV_PARENTS), 
			old_idx(idx_o),
			new_idx(idx_n),
			func_addr(addr){}
		/*!
		  \brief: �����㹻��ϢΪ�˱�����ԭʱʹ��
		  \return: void
		  \param e
		*/
		void save_in_info(cexpr_t* e)
		{
			cexpr_t *pParent = NULL;
			int n = parents.size();
			for (int i = 1; i < n; ++i) 
			{
				pParent = (cexpr_t *)parents.at(n-i);
				if (pParent->ea != 0xffffffff) // ֻҪ��ַ��Ϊ-1�͸ɻ�,����һֱ����ȡ
				{
					/// ������Ϣ
					restore_info info = {old_idx, pParent->op, pParent->ea, func_addr};
					if (!res_map_info.has(info))  // ���δ�ҵ������
					{
						res_map_info.push_back(info);
						// �洢�����ݿ�
						res_node.setblob(&res_map_info[0], res_map_info.size()*sizeof(restore_info), 0, 'J');
						res_node.altset(-1, res_map_info.size());
					}
					break;
				}
			} // end for with i < n
		}

		/*!
		  \brief: �������оֲ�����,�滻����
		  \return: int idaapi
		  \param e
		*/
		int idaapi visit_expr(cexpr_t *e)
		{
			if (e->op == cot_var)
			{
				if (e->v.idx == old_idx) 
				{
					save_in_info(e);
					e->v.idx = new_idx;
				}
			}
			return 0; // continue enumeration
		}
	};

	lvars_t &lvars = *cfunc->get_lvars(); // ���б���
	// ��������Ҫ�޸ĵı���,�޸�֮
	qvector<var_info>::iterator it = var_map_info.begin();
	for (; it != var_map_info.end(); ++it) 
	{
		if ((*it).func_addr == cfunc->entry_ea) 
		{
			if_inverter_t ifi((*it).old_name_index, (*it).new_name_index, cfunc->entry_ea);
			ifi.apply_to(&cfunc->body, NULL); 
			// �ó�δʹ��
			lvars[(*it).old_name_index].clear_used();
		}
	}
	return;
}

/*!
  \brief: ���ر���ӳ��ѡ��������
  \return: uint32 ����
  \param obj
*/
static uint32 idaapi map_var_sizer(void *obj)
{
	netnode *node = (netnode *)obj;
	return (uint32)node->altval(-1); // ������
}

/*!
  \brief: ���ÿ������
  \return: void
  \param obj
  \param n ��ǰ��
  \param arrptr ����
*/
static void idaapi map_var_desc(void *obj,uint32 n,char * const *arrptr)
{
	if ( n == 0 ) // ����
	{
		for ( int i=0; i < qnumber(header); i++ )
			qstrncpy(arrptr[i], header[i], MAXSTR);
		return;
	}
	netnode *node = (netnode *)obj;
	lvar_t& lavr = *(lvar_t*)node->altval(n-1);
	
	if (!lavr._type.empty())
	{
		print_type_to_one_line(arrptr[0], MAXSTR, idati, lavr._type.c_str());
	}
	qsnprintf(arrptr[1], MAXSTR, "%s", lavr.name.c_str());
}

/*!
  \brief: ��ѡ��"ӳ�����"�˵���ʱ�����
  \return: bool 
  \param ud
*/
bool idaapi map_var_to(void *ud)
{
	vdui_t &vu = *(vdui_t *)ud;
	lvars_t &lvars = *vu.cfunc->get_lvars();
	// 
	netnode* node = new netnode;
	node->create();
	
	// 2: ͳ������ʹ���еı�������
	int total_count = 0;
	lvars_t::iterator it = lvars.begin();
	for (int i = 0; it != lvars.end(); ++it, ++i) 
	{
		// ѡ�еı�������
		if (i == vu.item.e->v.idx) 
		{
			continue;
		}
		
		if ((*it).used()) 
		{
			node->altset(total_count, (nodeidx_t)&(*it));
			++total_count;
		}
	}
	/// �ܹ�����ʾ�������ŵ�-1������,�������ʱ��
	node->altset(-1, total_count);
	char szTitle[MAXSTR] = { 0 };
	qsnprintf(szTitle, MAXSTR, "map %s to", lvars[vu.item.e->v.idx].name.c_str());
	int choose_code = choose2(CH_MODAL,                    // modal window
		-1, -1, -1, -1,       // position is determined by Windows
		node,                 // ����
		qnumber(header),      // number of columns
		widths,               // widths of columns
		map_var_sizer,                // function that returns number of lines
		map_var_desc,                 // function that generates a line
		szTitle,         // window title
		-1,                   // use the default icon for the window
		0,                    // position the cursor on the first line
		NULL,                 // "kill" callback
		NULL,                 // "new" callback
		NULL,                 // "update" callback
		NULL,                 // "edit" callback
		NULL,                 // function to call when the user pressed Enter
		NULL,                 // function to call when the window is closed
		NULL,                 // use default popup menu items
		NULL);                // use the same icon for all lines
	if (choose_code <= 0) // ľ��ѡ��
	{
		node->kill();
		delete node;
		return true;
	}
	// ��ѡ�е�����ת�ɱ�������
	lvar_t& choose_var = *(lvar_t*)node->altval(choose_code-1);
	
	int idx = 0;
	for (it = lvars.begin(); it != lvars.end(); ++it, ++idx) 
	{
		if (choose_var == (*it)) 
		{
			break;
		}
	}
	// ��������
	var_info info = {idx, vu.item.e->v.idx, vu.cfunc->entry_ea};
	var_map_info.push_back(info);
	// �޸�����
	change_var_name(vu.cfunc);
	// �洢�����ݿ�
	var_node.setblob(&var_map_info[0], var_map_info.size()*sizeof(var_info), 0, 'S');
	var_node.altset(-1, var_map_info.size());
	
	// ˢ����������ctree�ṹ,
	vu.refresh_view(false);
	node->kill();
	delete node;
	return true;
}
//////////////////////////////////////////////////////////////////////////
// ������ӳ��
//////////////////////////////////////////////////////////////////////////
/*!
  \brief: �жϵ�ǰ��괦�����Ƿ��ܷ�ӳ��
  \return: bool �в���?
  \param vu
*/
bool is_var_can_unmap(vdui_t &vu)
{
	qvector<var_info>::iterator it = var_map_info.begin();
	for (; it != var_map_info.end(); ++it) 
	{
		if ((*it).new_name_index == vu.item.e->v.idx && (*it).func_addr == vu.cfunc->entry_ea) 
		{
			return true;
		}
	}
	return false;
}

/*!
  \brief: ���ص�ǰҪ������ĺ����Ƿ��������ǵĲ�������
  \return: bool
  \param ea
*/
bool is_func_in_list(ea_t ea)
{
	qvector<var_info>::iterator it = var_map_info.begin();
	for (; it != var_map_info.end(); ++it) 
	{
		if ((*it).func_addr == ea) 
		{
			return true;
		}
	}
	return false;
}

/*!
  \brief: ��ԭ������
  \return: void
  \param cfunc
  \param info
*/
void restore_var_name(cfunc_t * cfunc, var_info &info)
{
	/*! \brief �õ��������޸����б���*/
	struct if_inverter_t : public ctree_visitor_t
	{
		cfunc_t* m_func;
		qstring disp_name;
		int new_idx;
		if_inverter_t(cfunc_t* func, qstring name, int idx_n) : 
		ctree_visitor_t(CV_PARENTS), 
			m_func(func), 
			disp_name(name),
			new_idx(idx_n){}
		/*!
		  \brief: Ҫ��ԭ�Ĳ�����������ʽַ�Ƿ���ͬ
		  \return: bool
		  \param e
		*/
		bool is_restore_info_equal(cexpr_t* e)
		{
			cexpr_t *pParent = NULL;
			int n = parents.size();
			// ���ݵ�ǰ���ʽ�ҵ���һ�����õĸ����ʽ
			for (int i = 1; i < n; ++i) 
			{
				pParent = (cexpr_t *)parents.at(n-i);
				if (pParent->ea != 0xffffffff) // ֻҪ��ַ��Ϊ-1��һֱ����ȡ
				{
					break;
				}
			} // end for with i < n
			if (pParent == NULL) 
			{
				return false;
			}
			
			// �����Ѵ洢����Ϣ����ƥ��,�ҵ��򷵻�true
			qvector<restore_info>::iterator it = res_map_info.begin();
			for (; it != res_map_info.end(); ++it) 
			{
				restore_info& info = (*it);
				if (info.func_addr == m_func->entry_ea && 
					info.old_idx == new_idx && 
					info.op == pParent->op && 
					info.defea == pParent->ea) 
				{
					// �����ݿ���ɾ����Ӧ����
					res_map_info.del(info);
					res_node.setblob(&res_map_info[0], res_map_info.size()*sizeof(restore_info), 0, 'J');
					res_node.altset(-1, res_map_info.size());
					return true;
				}
			}
			return false;
		}
		int idaapi visit_expr(cexpr_t *e)
		{
			if (e->op == cot_var)
			{
				lvar_t &var = m_func->get_lvars()->at(e->v.idx);
				if (var.name == disp_name && is_restore_info_equal(e)) 
				{
					e->v.idx = new_idx;
				}
			}
			return 0; // continue enumeration
		}
	};

	// ��ԭ������
	qstring name = cfunc->get_lvars()->at(info.new_name_index).name;
	if_inverter_t ifi(cfunc, name, info.old_name_index);
	ifi.apply_to(&cfunc->body, NULL); 
	return;
}

/*!
  \brief: �����ķ�ӳ��
  \return: bool 
  \param ud
*/
bool idaapi ummap_var_from(void *ud)
{
	vdui_t &vu = *(vdui_t *)ud;
	// �½����ݶ���,�����ʾ����
	netnode* node = new netnode;
	node->create();

	// �������ݿ�,������ӳ�䵽�˱����ı�����ȡ��
	qvector<int> vec_old_idx;
	int total_count = 0;
	lvars_t &lvars = *vu.cfunc->get_lvars();
	qvector<var_info>::iterator it = var_map_info.begin();
	for (; it != var_map_info.end(); ++it) 
	{
		if ((*it).new_name_index == vu.item.e->v.idx && (*it).func_addr == vu.cfunc->entry_ea) 
		{
			node->altset(total_count, (nodeidx_t)&lvars[(*it).old_name_index]);
			vec_old_idx.push_back((*it).old_name_index);
			++total_count;
		}
	}
	node->altset(-1, total_count); // ����
	char szTitle[MAXSTR] = { 0 };
	qsnprintf(szTitle, MAXSTR, "unmap var from %s", lvars[vu.item.e->v.idx].name.c_str());
	int choose_code = choose2(
		CH_MODAL, 
		-1, -1, -1, -1, 
		node, 
		qnumber(header), 
		widths, 
		map_var_sizer, 
		map_var_desc, 
		szTitle, 
		-1, 0, 
		NULL, NULL, NULL, NULL, 
		NULL, NULL, NULL, NULL);
	if (choose_code <= 0) // ľ��ѡ��
	{
		node->kill();
		delete node;
		return true;
	}
	int index = vec_old_idx.at(choose_code - 1);
	qvector<var_info>::iterator ittmp = var_map_info.begin();
	for (; ittmp != var_map_info.end(); ++ittmp) 
	{
		if ((*ittmp).old_name_index == index && (*ittmp).func_addr == vu.cfunc->entry_ea) 
		{
			break;
		}
	}
	// �����ݿ���ɾ���ñ�����ӳ������
	var_map_info.del(*ittmp);
	var_node.setblob(&var_map_info[0], var_map_info.size()*sizeof(var_info), 0, 'S');
	var_node.altset(-1, var_map_info.size());

	// ��ԭ������
	lvars[(*ittmp).old_name_index].set_used();
	restore_var_name(vu.cfunc, *ittmp);	
	// ������������ctree
	vu.refresh_view(false);
	node->kill();
	delete node;
	return true;
}