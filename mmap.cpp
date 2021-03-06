//
// Created by feng on 2021/3/4.
//
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <string>
#include <stack>
using namespace std;
 struct TreeNode {
         int val;
         TreeNode *left;
         TreeNode *right;
         TreeNode() : val(0), left(nullptr), right(nullptr) {}
         TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
         TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
     };
    TreeNode* buildTree(vector<int>& preorder, vector<int>& inorder) {
        TreeNode* root = new TreeNode(preorder.front());

        if (preorder.size()==1)
        {
            return root;
        }

        int i,j;// i 表示跟节点，j之前的数是i节点的左子树；
        for (i = 0; i < inorder.size(); i++)
        {
            if (inorder.at(i) == preorder.front())
            {
                break;
            }
        }
        if (i==0)
        {
            j = 0;
        }else
        {
            for (j = 1; j < preorder.size(); j++)
            {
                if (preorder.at(j) == inorder.at(i-1))
                {
                    break;
                }
            }
        }
        vector<int>::iterator ds;
        if (i!=0)//有左子树
        {
            vector<int> p1 (preorder.begin()+1,preorder.begin()+j+1);
            vector<int> p2 (inorder.begin(),inorder.begin()+i);
            root->left = buildTree(p1,p2);
        }
        if (i!=inorder.size()-1)//有右子树
        {
            vector<int> p3 (preorder.begin()+j+1,preorder.end());
            vector<int> p4 (inorder.begin()+i+1,inorder.end());
            root->right = buildTree(p3,p4);
        }

        return root;
    }


    int Balanced(TreeNode* root, int in)
    {
        int left = Balanced(root->left,in+1);
        int right = Balanced(root->right,in+1);
        if(left==-1 || right ==-1) return -1;
        if(abs(left-right)>1) return -1;
        else return max(left,right);
    }
    bool isBalanced(TreeNode* root) {
        if (!root) return true;
        if(Balanced(root,0) == -1)
            return false;
        else return true;
    }
 struct ListNode {
         int val;
         ListNode *next;
         ListNode() : val(0), next(nullptr) {}
         ListNode(int x) : val(x), next(nullptr) {}
         ListNode(int x, ListNode *next) : val(x), next(next) {}
     };



void flatten1(TreeNode* root) {
    TreeNode* ptr;
    TreeNode* ptr1 = root;
    stack<TreeNode*> st;
    st.push(nullptr);
    ptr = root;
    while (ptr)
    {
        if (ptr->right)
        {
            st.push(ptr->right);
            ptr->right = nullptr;
        }
        if (ptr->left)
        {
            ptr = ptr->left;
            ptr1 = ptr;
        }
        else
        {
            ptr1->left = st.top();
            ptr = st.top();
            ptr1 =ptr;
            st.pop();
        }
    }
}
void flatten(TreeNode* root) {
    TreeNode* ptr,*temp;
    //TreeNode* ptr1 = root;
    stack<TreeNode*> st;
    st.push(nullptr);
    ptr = root;
    while (ptr)
    {
        temp = ptr->right;
        ptr->right = ptr->left;
        ptr->left = temp;

        if (ptr->left)
        {
            st.push(ptr->left);
            ptr->left = nullptr;
        }
        if (ptr->right)
        {
            ptr = ptr->right;
            //ptr1 = ptr;
        }
        else
        {
            ptr->right = st.top();
            ptr = st.top();
            //ptr1 =ptr;
            st.pop();
        }
    }
}

class Solution {
public:
    vector<vector<int>> result;
    void fun(TreeNode* root, int const targetSum, int temp, vector<int>& path)
    {
        if(!root) return;
        path.push_back(root->val);
        temp += root->val;
        if(temp == targetSum && !root->left && !root->right)
        {
            result.push_back(path);
        }else
        {
            fun(root->left, targetSum, temp, path);
            fun(root->right, targetSum, temp, path);
        }
     }
    vector<vector<int>> pathSum(TreeNode* root, int targetSum) {
        vector<int> s;
        fun(root,targetSum,0,s);
        return result;
    }
};
int main()
{
    Solution s;
    TreeNode *t = new TreeNode(0,new TreeNode(1,new TreeNode(4),new TreeNode(7)),new TreeNode(3,new TreeNode(9),new TreeNode(6)));
    vector<vector<int>> result = s.pathSum(t,5);
    int   add = 1;
}
