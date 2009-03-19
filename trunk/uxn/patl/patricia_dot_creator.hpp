#ifndef PATL_PATRICIA_DOT_CREATOR_HPP
#define PATL_PATRICIA_DOT_CREATOR_HPP

#include <iostream>
#include <iomanip>

namespace uxn
{
namespace patl
{

template <typename Cont, typename OutStream = std::ostream>
class patricia_dot_creator
{
    patricia_dot_creator &operator=(const patricia_dot_creator&);

    typedef Cont cont_type;
    typedef typename cont_type::vertex vertex;
    typedef typename cont_type::preorder_iterator preorder_iterator;
    typedef typename cont_type::bit_compare bit_compare;
    typedef typename cont_type::prefix prefix;

    static const word_t bit_size = bit_compare::bit_size;

public:
    patricia_dot_creator(OutStream &out = std::cout, word_t dpi = 96)
        : out_(out)
    {
        out_ << "strict digraph\n";
        out_ << "{\n";
        out_ << "// common props\n";
        out_ << "dpi = " << dpi << "\n";
        out_ << "edge[arrowhead = empty]\n";
    }

    ~patricia_dot_creator()
    {
        out_ << "}\n";
    }

    void create(const vertex &vtx, bool clustering = false) const
    {
        out_ << "\n// BEGIN create from vertex\n";
        out_ << "\n// node definitions (preorder depth-first)\n";
        const word_t
            init_id = vtx.get_qid(),
            sibl_qtag = vtx.sibling().get_qtag();
        out_ << "sibling[shape = plaintext, label = "
            << (vtx.skip() == ~word_t(0) ? "\"nil\"" : "\"...\"")
            << "]\n";
        const preorder_iterator pit_end(vtx.preorder_end());
        for (preorder_iterator pit(vtx.preorder_begin()); pit != pit_end; ++pit)
        {
            if (init_id == pit->get_qid())
                out_ << 'n' << std::hex << pit->node_q_uid() << std::dec
                    << "[label = \""
                    << pit->parent_key()
                    << "\\n"
                    << static_cast<sword_t>(pit->skip())
                    << "\"]\n";
        }
        //
        out_ << std::hex;
        //
        out_ << "\n// root link to sibling\n";
        out_ << 'n' << vtx.node_q_uid() << "->sibling[tailport = "
            << (init_id ? "sw" : "se") << ", headport = "
            << (sibl_qtag ? (init_id ? "se" : "sw") : (init_id ? "ne" : "nw")) << ", style = "
            << (sibl_qtag ? "dotted" : "solid") << "]\n";
        out_ << "\n// forward left links\n";
        out_ << "edge[tailport = sw, weight = 2]\n";
        {
            bool linkp = false;
            for (preorder_iterator pit(vtx.preorder_begin()); pit != pit_end; ++pit)
            {
                if (linkp)
                    out_ << "->" << 'n' << pit->node_q_uid();
                if (!pit->get_qtag() && !pit->get_qid())
                {
                    if (!linkp)
                        out_ << 'n' << pit->node_q_uid();
                    linkp = true;
                }
                else
                {
                    if (linkp)
                        out_ << "\n";
                    linkp = false;
                }
            }
        }
        out_ << "\n// forward right links\n";
        out_ << "edge[tailport = se]\n";
        {
            word_t prev_node = 0;
            for (preorder_iterator pit(vtx.preorder_begin()); pit != pit_end; ++pit)
            {
                if (!pit->get_qtag() && pit->get_qid())
                {
                    if (prev_node != pit->node_q_uid())
                    {
                        if (prev_node)
                            out_ << '\n';
                        out_ << 'n' << pit->node_q_uid();
                    }
                    prev_node = pit->node_p_uid();
                    out_ << "->" << 'n' << prev_node;
                }
            }
            if (prev_node)
                out_ << '\n';
        }
        out_ << "\n// backward left links\n";
        out_ << "edge[style = dotted, tailport = sw, weight = 1]\n";
        for (preorder_iterator pit(vtx.preorder_begin()); pit != pit_end; ++pit)
        {
            if (pit->get_qtag() && !pit->get_qid())
                out_ << 'n' << pit->node_q_uid() << "->" << 'n' << pit->node_p_uid() << "\n";
        }
        out_ << "\n// backward right links\n";
        out_ << "edge[tailport = se]\n";
        for (preorder_iterator pit(vtx.preorder_begin()); pit != pit_end; ++pit)
        {
            if (pit->get_qtag() && pit->get_qid())
                out_ << 'n' << pit->node_q_uid() << "->" << 'n' << pit->node_p_uid() << "\n";
        }
        if (clustering)
        {
            out_ << "\n// clustering nodes by symbols (recursive levelorder traversal)\n";
            word_t n = 0;
            out_cluster(n, vtx.get_prefix());
        }
        out_ << "\n// END create from vertex\n";
    }

private:
    void out_cluster(word_t &n, const prefix &root) const
    {
        const bool clustp =
            !root.get_xtag(0) && !root.symbol_limit(0) ||
            !root.get_xtag(1) && !root.symbol_limit(1);
        if (clustp)
            out_ << "subgraph cluster_" << n++ << "\n"
                << "{ style = \"dashed, rounded, setlinewidth(0.2)\"; ";
        std::vector<prefix> children;
        for (prefix cur(root); ; )
        {
            if (!cur.get_xtag(0) && cur.symbol_limit(0))
            {
                prefix pref(cur);
                pref.go_xlink(0);
                children.push_back(pref);
            }
            if (!cur.get_xtag(1) && cur.symbol_limit(1))
            {
                prefix pref(cur);
                pref.go_xlink(1);
                children.push_back(pref);
            }
            if (clustp)
            {
                if (root != cur)
                    out_ << "; ";
                out_ << 'n' << cur.node_uid();
            }
            //
            if (!cur.get_xtag(0) && !cur.symbol_limit(0))
                cur.go_xlink(0);
            else if (!cur.get_xtag(1) && !cur.symbol_limit(1))
                cur.go_xlink(1);
            else
            {
                if (cur == root)
                    break;
                for (;;)
                {
                    word_t parent_id;
                    do
                    {
                        parent_id = cur.get_parent_id();
                        cur.go_parent();
                    } while (cur != root && parent_id);
                    if (cur == root)
                        break;
                    if (!cur.get_xtag(1) && !cur.symbol_limit(1))
                    {
                        cur.go_xlink(1);
                        break;
                    }
                }
                if (cur == root)
                    break;
            }
        }
        if (clustp)
            out_ << " }\n";
        //
        for (word_t i = 0; i != children.size(); ++i)
            out_cluster(n, children[i]);
    }

    OutStream &out_;
};

} // namespace patl
} // namespace uxn

#endif