/*****************************************************************************
 * tfind.c : implement every t* fuctions
 *****************************************************************************/

#if defined(_WIN32) || defined(__ANDROID__)
#include <vlc_fixups.h>
#include <vlc_common.h>

#include <assert.h>
#include <stdlib.h>

typedef struct node {
    char         *key;
    struct node  *llink, *rlink;
} node_t;

/*
 * Tree search generalized from Knuth (6.2.2) Algorithm T just like
 * the AT&T man page says.
 *
 * The node_t structure is for internal use only, lint doesn't grok it.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */

/* delete node with given key */
void *tdelete(const void *vkey, void **vrootp, int(*compar)(const void *, const void *))
{
	node_t **rootp = (node_t **)vrootp;
	node_t *p, *q, *r;
	int  cmp;

	assert(vkey != NULL);
	assert(compar != NULL);

	if (rootp == NULL || (p = *rootp) == NULL)
		return NULL;

	while ((cmp = (*compar)(vkey, (*rootp)->key)) != 0) 
    {
		p = *rootp;
		rootp = (cmp < 0) ? &(*rootp)->llink : &(*rootp)->rlink;
		if (*rootp == NULL)
			return NULL;
	}
	r = (*rootp)->rlink;			/* D1: */
	if ((q = (*rootp)->llink) == NULL)	/* Left NULL? */
		q = r;
	else if (r != NULL)
    {
		if (r->llink == NULL)
        {
			r->llink = q;
			q = r;
		}
        else
        {
			for (q = r->llink; q->llink != NULL; q = r->llink)
				r = q;
			r->llink = q->rlink;
			q->llink = (*rootp)->llink;
			q->rlink = (*rootp)->rlink;
		}
	}
    if (p != *rootp)
        free(*rootp);
    else if (q == NULL) // K.C.Y
        free(*rootp);
	*rootp = q;

	return p;
}


/*
 * Tree search generalized from Knuth (6.2.2) Algorithm T just like
 * the AT&T man page says.
 *
 * The node_t structure is for internal use only, lint doesn't grok it.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */

/* Walk the nodes of a tree */
static void tdestroy_recurse(node_t* root, void (*free_action)(void *))
{
    if (root->llink != NULL)
        tdestroy_recurse(root->llink, free_action);
    if (root->rlink != NULL)
        tdestroy_recurse(root->rlink, free_action);

    (*free_action)((void *)root->key);
    free(root);
}

void tdestroy(void *vrootp, void (*freefct)(void *))
{
    node_t *root = (node_t *)vrootp;

    if (root != NULL)
        tdestroy_recurse(root, freefct);
}


/*
 * Tree search generalized from Knuth (6.2.2) Algorithm T just like
 * the AT&T man page says.
 *
 * The node_t structure is for internal use only, lint doesn't grok it.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */

/* find a node, or return 0 */
void *tfind(const void *vkey, const void **vrootp, int (*compar)(const void *, const void *))
{
	node_t * const *rootp = (node_t * const*)vrootp;

	assert(vkey != NULL);
	assert(compar != NULL);

	if (rootp == NULL)
		return NULL;

	while (*rootp != NULL)
    {
		int r;

		if ((r = (*compar)(vkey, (*rootp)->key)) == 0)
			return *rootp;

		rootp = (r < 0) ? &(*rootp)->llink : &(*rootp)->rlink;
	}
	return NULL;
}


/*
 * Tree search generalized from Knuth (6.2.2) Algorithm T just like
 * the AT&T man page says.
 *
 * The node_t structure is for internal use only, lint doesn't grok it.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */

/* find or insert datum into search tree */
void* tsearch(const void *vkey, void **vrootp, int (*compar)(const void *, const void *))
{
	node_t *q;
	node_t **rootp = (node_t **)vrootp;

	assert(vkey != NULL);
	assert(compar != NULL);

	if (rootp == NULL)
		return NULL;

	while (*rootp != NULL)
    {
		int r;

		if ((r = (*compar)(vkey, (*rootp)->key)) == 0)
			return *rootp;

		rootp = (r < 0) ? &(*rootp)->llink : &(*rootp)->rlink;
	}

	q = malloc(sizeof(node_t));
	if (q != 0)
    {
		*rootp = q;
		q->key = (void*)vkey;
		q->llink = q->rlink = NULL;
        
        //msg_Dbg(NULL, "Node alloc: 0x%p", q);
	}

	return q;
}


/*
 * Tree search generalized from Knuth (6.2.2) Algorithm T just like
 * the AT&T man page says.
 *
 * The node_t structure is for internal use only, lint doesn't grok it.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */

/* Walk the nodes of a tree */
static void twalk_recurse(const node_t *root, void (*action)(const void *, VISIT, int), int level)
{
	assert(root != NULL);
	assert(action != NULL);

	if (root->llink == NULL && root->rlink == NULL)
		(*action)(root, leaf, level);
	else
    {
		(*action)(root, preorder, level);
		if (root->llink != NULL)
			twalk_recurse(root->llink, action, level + 1);
		(*action)(root, postorder, level);
		if (root->rlink != NULL)
			twalk_recurse(root->rlink, action, level + 1);
		(*action)(root, endorder, level);
	}
}

/* Walk the nodes of a tree */
void twalk(const void *vroot, void (*action)(const void *, VISIT, int))
{
	if (vroot != NULL && action != NULL)
		twalk_recurse(vroot, action, 0);
}
#endif