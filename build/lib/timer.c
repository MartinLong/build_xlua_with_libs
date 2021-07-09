#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "lua.h"
#include "lauxlib.h"

#define TIME_NEAR_SHIFT 8
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_LEVEL_SHIFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR-1)
#define TIME_LEVEL_MASK (TIME_LEVEL-1)

struct timer_node {
    struct timer_node *next;
    int expire;
    int session;
};

struct link_list {
    struct timer_node head;
    struct timer_node *tail;
};

struct timer {
    struct link_list near[TIME_NEAR];
    struct link_list t[4][TIME_LEVEL-1];
    int time;
    struct timer_node * free_list;
    int count;
};

static inline struct timer_node *
link_clear(struct link_list *list)
{
    struct timer_node * ret = list->head.next;
    list->head.next = 0;
    list->tail = &(list->head);

    return ret;
}

static inline void
link(struct link_list *list,struct timer_node *node)
{
    list->tail->next = node;
    list->tail = node;
    node->next=0;
}

static void
add_node(struct timer *T, struct timer_node *node)
{
    int time=node->expire;
    int current_time=T->time;

    if ((time|TIME_NEAR_MASK)==(current_time|TIME_NEAR_MASK)) {
        link(&T->near[time&TIME_NEAR_MASK],node);
    }
    else {
        int i;
        int mask=TIME_NEAR << TIME_LEVEL_SHIFT;
        for (i=0;i<3;i++) {
            if ((time|(mask-1))==(current_time|(mask-1))) {
                break;
            }
            mask <<= TIME_LEVEL_SHIFT;
        }
        link(&T->t[i][((time>>(TIME_NEAR_SHIFT + i*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)-1],node);
    }
}

static struct timer_node *
alloc_node(struct timer *T) {
    struct timer_node * node = T->free_list;
    if(node) {
        T->free_list = node->next;
        return node;
    }

    T->count++;
    return malloc(sizeof(*node));
}

static void
timer_add(struct timer *T,int session,int time)
{
    struct timer_node *node = alloc_node(T);
    node->session = session;

    node->expire=time+T->time;
    add_node(T,node);
}

static int
timer_execute(struct timer *T, lua_State *L, int t)
{
    int idx=T->time & TIME_NEAR_MASK;
    int mask,i,time;
    struct link_list * list = &T->near[idx];
    struct timer_node * node = list->head.next;
    int n = 0;

    if(node) {
        while(node) {
            lua_pushinteger(L, node->session);
            lua_rawseti(L, t, ++n);
            node = node->next;
        }

        list->tail->next = T->free_list;
        T->free_list = list->head.next;

        link_clear(list);
    }

    ++T->time;

    mask = TIME_NEAR;
    time = T->time >> TIME_NEAR_SHIFT;
    i=0;

    while ((T->time & (mask-1))==0) {
        idx=time & TIME_LEVEL_MASK;
        if (idx!=0) {
            --idx;
            struct timer_node *current=link_clear(&T->t[i][idx]);
            while (current) {
                struct timer_node *temp=current->next;
                add_node(T,current);
                current=temp;
            }
            break;
        }
        mask <<= TIME_LEVEL_SHIFT;
        time >>= TIME_LEVEL_SHIFT;
        ++i;
    }

    return n;
}

static struct timer *
timer_create()
{
    struct timer *T=(struct timer *)malloc(sizeof(struct timer));
    memset(T,0,sizeof(*T));

    int i,j;

    for (i=0;i<TIME_NEAR;i++) {
        link_clear(&T->near[i]);
    }

    for (i=0;i<4;i++) {
        for (j=0;j<TIME_LEVEL-1;j++) {
            link_clear(&T->t[i][j]);
        }
    }

    T->free_list = NULL;

    return T;
}

static void
link_free(struct timer *T, struct link_list *list)
{
    struct timer_node * p = list->head.next;
    while (p) {
        struct timer_node *tmp = p;
        p = p->next;
        T->count--;
        free(tmp);
    }
}

static void
timer_release(struct timer * T) {
    int i,j;

    struct timer_node * node = T->free_list;
    while(node) {
        struct timer_node * tmp = node;
        node = node->next;
        T->count--;
        free(tmp);
    }

    for (i=0;i<TIME_NEAR;i++) {
        link_free(T, &T->near[i]);
    }

    for (i=0;i<4;i++) {
        for (j=0;j<TIME_LEVEL-1;j++) {
            link_free(T, &T->t[i][j]);
        }
    }

    assert(T->count == 0);
    free(T);
}

static inline struct timer *
get_timer(lua_State *L) {
    struct timer ** v = lua_touserdata(L, 1);
    return *v;
}

static int
lrelease(lua_State *L) {
    struct timer * t = get_timer(L);
    timer_release(t);
    return 0;
}

static int
ladd(lua_State *L) {
    struct timer * t = get_timer(L);
    int time = luaL_checkinteger(L,2);
    int session = luaL_checkinteger(L,3);

    timer_add(t, session, time);

    lua_pushinteger(L, t->time + time);
    return 1;
}

static int
lexecute(lua_State *L) {
    struct timer * t = get_timer(L);
    luaL_checktype(L, 2, LUA_TTABLE);
    int n = timer_execute(t, L, 2);
    lua_pushinteger(L, n);
    return 1;
}

static int
lframe(lua_State *L) {
    struct timer * t = get_timer(L);
    lua_pushinteger(L, t->time);
    return 1;
}

static int
lcreate_timer(lua_State *L) {
    struct timer ** v = lua_newuserdata(L,sizeof(struct timer *));
    *v = timer_create();
    if (luaL_newmetatable(L, "timer")) {
        lua_pushcfunction(L, lrelease);
        lua_setfield(L, -2, "__gc");
        luaL_Reg l[] = {
            { "execute", lexecute },
            { "add", ladd },
            { "frame", lframe },
            { NULL, NULL },
        };
        luaL_newlib(L, l);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L,-2);

    return 1;
}

LUAMOD_API int
luaopen_timer_c(lua_State *L) {
    luaL_checkversion(L);
    lua_pushcfunction(L, lcreate_timer);
    return 1;
}

