#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <Python.h>

#include "libipvs.h"
#include "ip_vs.h"

#define DEF_MCAST_IFN  "eth0"
#define DEF_SCHED      "wlc"

#define PyDict_ContainsString(dict, key) PyDict_Contains(dict, PyString_FromString(key))

#define PyDict_SetStringLong(dict, key, value) PyDict_SetItemString(dict, key, PyInt_FromLong(value))
#define PyDict_GetStringLong(dict, key) PyInt_AsLong(PyDict_GetItemString(dict, key))

#define PyDict_SetStringString(dict, key, value) PyDict_SetItemString(dict, key, PyString_FromString(value))
#define PyDict_GetStringString(dict, key) PyString_AsString(PyDict_GetItemString(dict, key))


static PyObject*
pyipvs_stats_to_dict(struct ip_vs_stats_user *stats) {
    PyObject *statdict;

    statdict = PyDict_New();

    PyDict_SetStringLong(statdict, "conns", stats->conns);
    PyDict_SetStringLong(statdict, "inpkts", stats->inpkts);
    PyDict_SetStringLong(statdict, "outpkts", stats->outpkts);
    PyDict_SetStringLong(statdict, "inbytes", stats->inbytes);
    PyDict_SetStringLong(statdict, "outbytes", stats->outbytes);
    PyDict_SetStringLong(statdict, "cps", stats->cps);
    PyDict_SetStringLong(statdict, "inpps", stats->inpps);
    PyDict_SetStringLong(statdict, "outpps", stats->outpps);
    PyDict_SetStringLong(statdict, "inbps", stats->inbps);
    PyDict_SetStringLong(statdict, "outbps", stats->outbps);

    return statdict;
}


static PyObject*
pyipvs_timeout_to_dict(ipvs_timeout_t *timeout) {
    PyObject *timeoutdict;

    timeoutdict = PyDict_New();

    PyDict_SetStringLong(timeoutdict, "tcp", timeout->tcp_timeout);
    PyDict_SetStringLong(timeoutdict, "tcp_fin", timeout->tcp_fin_timeout);
    PyDict_SetStringLong(timeoutdict, "udp", timeout->udp_timeout);

    return timeoutdict;
}


static void
pyipvs_dict_to_timeout(PyObject *timeoutdict, ipvs_timeout_t *timeout) {

    timeout->tcp_timeout = PyDict_GetStringLong(timeoutdict, "tcp");
    timeout->tcp_fin_timeout = PyDict_GetStringLong(timeoutdict, "tcp_fin");
    timeout->udp_timeout = PyDict_GetStringLong(timeoutdict, "udp");
}


static PyObject*
pyipvs_daemon_to_dict(ipvs_daemon_t *daemon) {
    PyObject *daemondict;

    daemondict = PyDict_New();

    if(daemon->state & IP_VS_STATE_NONE)
        PyDict_SetStringString(daemondict, "state", "none");
    else if(daemon->state & IP_VS_STATE_MASTER)
        PyDict_SetStringString(daemondict, "state", "master");
    else if(daemon->state & IP_VS_STATE_BACKUP)
        PyDict_SetStringString(daemondict, "state", "backup");
    /* FIXME(mbra) raise exception */

    PyDict_SetStringLong(daemondict, "syncid", daemon->syncid);

    PyDict_SetStringString(daemondict, "iface", daemon->mcast_ifn);

    return daemondict;
}


static void
pyipvs_dict_to_daemon(PyObject* daemondict, ipvs_daemon_t *daemon) {
    char *state = PyString_AsString(PyDict_GetItemString(daemondict, "state"));

    if(strcasecmp(state, "none") == 0)
        daemon->state = IP_VS_STATE_NONE;
    else if(strcasecmp(state, "master") == 0)
        daemon->state = IP_VS_STATE_MASTER;
    else if(strcasecmp(state, "backup") == 0)
        daemon->state = IP_VS_STATE_BACKUP;

    daemon->syncid = PyDict_GetStringLong(daemondict, "syncid");

    strncpy(daemon->mcast_ifn, PyDict_GetStringString(daemondict, "iface"), IP_VS_IFNAME_MAXLEN);

}


static PyObject*
pyipvs_dest_to_dict(ipvs_dest_entry_t *dest) {
    struct in_addr addr;
    addr.s_addr = dest->addr;
    PyObject *destdict;

    destdict = PyDict_New();
    PyDict_SetStringString(destdict, "address" , inet_ntoa(addr));
    PyDict_SetStringLong(destdict, "port" , ntohs(dest->port));
    PyDict_SetStringLong(destdict, "weight" , dest->weight);

    if(dest->conn_flags & IP_VS_CONN_F_DROUTE)
        PyDict_SetStringString(destdict, "forward" , "droute");
    else if(dest->conn_flags & IP_VS_CONN_F_TUNNEL)
        PyDict_SetStringString(destdict, "forward" , "ipip");
    else if(dest->conn_flags & IP_VS_CONN_F_MASQ)
        PyDict_SetStringString(destdict, "forward" , "masq");


    PyDict_SetStringLong(destdict, "u_threshold", dest->u_threshold);
    PyDict_SetStringLong(destdict, "l_threshold", dest->l_threshold);

    PyDict_SetItemString(destdict, "stats", pyipvs_stats_to_dict(&dest->stats));

    return destdict;
}


static void
pyipvs_dict_to_dest(PyObject *destdict, ipvs_dest_t *dest) {
    struct in_addr addr;

    inet_aton(PyDict_GetStringString(destdict, "address"), &addr);
    dest->addr = addr.s_addr;

    dest->port = htons(PyDict_GetStringLong(destdict, "port"));

    if(PyDict_ContainsString(destdict, "weight"))
        dest->weight = PyDict_GetStringLong(destdict, "weight");
    else
        dest->weight = 0;

    if(PyDict_ContainsString(destdict, "u_threshold"))
        dest->u_threshold = PyDict_GetStringLong(destdict, "u_threshold");

    if(PyDict_ContainsString(destdict, "l_threshold"))
        dest->l_threshold = PyDict_GetStringLong(destdict, "l_threshold");

    if(PyDict_ContainsString(destdict, "forward")) {
        char *fw = PyDict_GetStringString(destdict, "forward");
        if(strcasecmp(fw, "droute") == 0)
            dest->conn_flags = IP_VS_CONN_F_DROUTE;
        else if (strcasecmp(fw, "ipip") == 0)
            dest->conn_flags = IP_VS_CONN_F_TUNNEL;
        else if (strcasecmp(fw, "masq") == 0)
            dest->conn_flags = IP_VS_CONN_F_MASQ;
        else
            exit(1); /* FIXME(mbra) raise exception */
    } else {
        dest->conn_flags = IP_VS_CONN_F_DROUTE;
    }

}


static PyObject*
pyipvs_service_to_dict(ipvs_service_entry_t *svc) {
    PyObject *svcdict;
    struct in_addr addr;
    addr.s_addr = svc->addr;

    svcdict = PyDict_New();

    if(svc->fwmark > 0) {
        PyDict_SetStringLong(svcdict, "fwmark" , svc->fwmark);
    } else {
        PyDict_SetStringString(svcdict, "protocol" , svc->protocol == IPPROTO_TCP ? "TCP" : "UDP");
        PyDict_SetStringString(svcdict, "address" ,inet_ntoa(addr));
        PyDict_SetStringLong(svcdict, "port" , ntohs(svc->port));
    }

    if(svc->flags & IP_VS_SVC_F_PERSISTENT)
        PyDict_SetStringLong(svcdict, "persistent", svc->timeout);

    addr.s_addr = svc->netmask;
    PyDict_SetStringString(svcdict, "netmask", inet_ntoa(addr));

    PyDict_SetStringString(svcdict, "scheduler" , svc->sched_name);

    PyDict_SetItemString(svcdict, "stats", pyipvs_stats_to_dict(&svc->stats));

    return svcdict;

}


static void
pyipvs_dict_to_service(PyObject *svcdict, ipvs_service_t *svc) {
    struct in_addr addr;

    if(PyDict_Contains(svcdict, PyString_FromString("netmask"))) {
        inet_aton(PyDict_GetStringString(svcdict, "netmask"), &addr);
        svc->netmask = addr.s_addr;
    } else {
        /* set 255.255.255.255 as default */
        svc->netmask = ((u_int32_t) 0xffffffff);
    }

    if(PyDict_ContainsString(svcdict, "persistent")) {
        svc->flags =  IP_VS_SVC_F_PERSISTENT;
        svc->timeout = PyDict_GetStringLong(svcdict, "persistent");
    }

    if(!PyDict_Contains(svcdict, PyString_FromString("fwmark"))) {
        char *protocol;

        inet_aton(PyDict_GetStringString(svcdict, "address"), &addr);
        svc->addr = addr.s_addr;

        svc->port = htons(PyDict_GetStringLong(svcdict, "port"));

        protocol = PyDict_GetStringString(svcdict, "protocol");

        if (strcasecmp(protocol, "TCP") == 0)
            svc->protocol = IPPROTO_TCP;
        else if (strcasecmp(protocol, "UDP") == 0)
            svc->protocol = IPPROTO_UDP;
        else
            svc->protocol = IPPROTO_TCP; /* set tcp as default */

        svc->fwmark = 0;
        /*    free(protocol); FIXME(mbra) why does this generate an error? */
    } else {
        svc->fwmark = PyDict_GetStringLong(svcdict, "fwmark");
        svc->protocol = IPPROTO_TCP;
    }

    if(PyDict_ContainsString(svcdict, "scheduler")) {
        strncpy(svc->sched_name, PyDict_GetStringString(svcdict, "scheduler"), IP_VS_SCHEDNAME_MAXLEN);
    } else {
        strncpy(svc->sched_name, DEF_SCHED, IP_VS_SCHEDNAME_MAXLEN);
    }
}


static PyObject*
pyipvs_version(PyObject *self, PyObject *args)
{
    unsigned int version = ipvs_version();

    PyObject *version_tuple = PyTuple_New(3);
    PyTuple_SetItem(version_tuple, 0, PyInt_FromLong(version >> 16 & 0xff));
    PyTuple_SetItem(version_tuple, 1, PyInt_FromLong(version >> 8 & 0xff));
    PyTuple_SetItem(version_tuple, 2, PyInt_FromLong(version & 0xff));

    return version_tuple;
}


static PyObject*
pyipvs_init(PyObject *self, PyObject *args) {
    int result = ipvs_init();
    return PyInt_FromLong(result);
}


static PyObject*
pyipvs_getinfo(PyObject *self, PyObject *args) {
    /* FIXME(mbra) This can't be right.... */
    ipvs_getinfo();
    Py_RETURN_NONE;
}


static PyObject*
pyipvs_start_daemon(PyObject *self, PyObject *args) {
    PyObject *daemondict;
    ipvs_daemon_t daemon;

    PyArg_ParseTuple(args, "O", &daemondict);
    pyipvs_dict_to_daemon(daemondict, &daemon);
    return Py_BuildValue("i", ipvs_start_daemon(&daemon));
}


static PyObject*
pyipvs_stop_daemon(PyObject *self, PyObject *args) {
    PyObject *daemondict;
    ipvs_daemon_t daemon;

    PyArg_ParseTuple(args, "O", &daemondict);
    pyipvs_dict_to_daemon(daemondict, &daemon);
    return Py_BuildValue("i", ipvs_stop_daemon(&daemon));
}


static PyObject*
pyipvs_get_daemon(PyObject *self, PyObject *args) {
    PyObject *daemondict;
    ipvs_daemon_t *daemon;

    daemon = ipvs_get_daemon();
    daemondict = pyipvs_daemon_to_dict(daemon);

    free(daemon);

    return daemondict;
}


static PyObject*
pyipvs_flush(PyObject *self, PyObject *args) {
    return Py_BuildValue("i", ipvs_flush());
}


static PyObject*
pyipvs_get_service(PyObject *self, PyObject *args) {
    PyObject *svcdict_qry, *svcdict;
    ipvs_service_t svc_qry;
    ipvs_service_entry_t *svc;
    PyArg_ParseTuple(args, "O", &svcdict_qry);

    pyipvs_dict_to_service(svcdict_qry, &svc_qry);

    svc = ipvs_get_service(svc_qry.fwmark, svc_qry.protocol, svc_qry.addr, svc_qry.port);
    svcdict = pyipvs_service_to_dict(svc);

    free(svc);

    return svcdict;
}


static PyObject*
pyipvs_get_services(PyObject *self, PyObject *args) {
    unsigned int i;
    struct ip_vs_get_services *get = ipvs_get_services();

    if (!get) return PyTuple_New(0);

    PyObject *services_tuple = PyTuple_New(get->num_services);

    for (i = 0; i < get->num_services; i++) {
        PyTuple_SetItem(services_tuple, i, pyipvs_service_to_dict(&get->entrytable[i]));
    }

    free(get);

    return services_tuple;

}


static PyObject*
pyipvs_add_service(PyObject *self, PyObject *args) {
    PyObject *svcdict;
    ipvs_service_t svc;
    int result;
    PyArg_ParseTuple(args, "O", &svcdict);

    pyipvs_dict_to_service(svcdict, &svc);

    result = ipvs_add_service(&svc);
    return Py_BuildValue("i", result);
}


static PyObject*
pyipvs_update_service(PyObject *self, PyObject *args) {
    PyObject *svcdict;
    ipvs_service_t svc;
    int result;
    PyArg_ParseTuple(args, "O", &svcdict);

    pyipvs_dict_to_service(svcdict, &svc);

    result = ipvs_update_service(&svc);
    return Py_BuildValue("i", result);
}


static PyObject*
pyipvs_del_service(PyObject *self, PyObject *args) {
    PyObject *svcdict;
    ipvs_service_t svc;
    int result;

    PyArg_ParseTuple(args, "O", &svcdict);

    pyipvs_dict_to_service(svcdict, &svc);
    result = ipvs_del_service(&svc);

    return Py_BuildValue("i", result);
}


static PyObject*
pyipvs_zero_service(PyObject *self, PyObject *args) {
    PyObject *svcdict;
    ipvs_service_t svc;
    int result;
    PyArg_ParseTuple(args, "O", &svcdict);

    pyipvs_dict_to_service(svcdict, &svc);

    result = ipvs_zero_service(&svc);
    return Py_BuildValue("i", result);
}


static PyObject*
pyipvs_add_dest(PyObject *self, PyObject *args) {
    PyObject *svcdict, *destdict;
    ipvs_service_t svc;
    ipvs_dest_t dest;
    int result;

    PyArg_ParseTuple(args, "OO", &svcdict, &destdict);

    pyipvs_dict_to_dest(destdict, &dest);
    pyipvs_dict_to_service(svcdict, &svc);

    result = ipvs_add_dest(&svc, &dest);

    return Py_BuildValue("i", result);
}


static PyObject*
pyipvs_update_dest(PyObject *self, PyObject *args) {
    PyObject *svcdict, *destdict;
    ipvs_service_t svc;
    ipvs_dest_t dest;
    int result;

    PyArg_ParseTuple(args, "OO", &svcdict, &destdict);

    pyipvs_dict_to_dest(destdict, &dest);
    pyipvs_dict_to_service(svcdict, &svc);

    result = ipvs_update_dest(&svc, &dest);

    return Py_BuildValue("i", result);
}


static PyObject*
pyipvs_del_dest(PyObject *self, PyObject *args) {
    PyObject *svcdict, *destdict;
    ipvs_service_t svc;
    ipvs_dest_t dest;
    int result;

    PyArg_ParseTuple(args, "OO", &svcdict, &destdict);

    pyipvs_dict_to_dest(destdict, &dest);
    pyipvs_dict_to_service(svcdict, &svc);

    result = ipvs_del_dest(&svc, &dest);

    return Py_BuildValue("i", result);
}


static PyObject*
pyipvs_get_dests(PyObject *self, PyObject *args) {
    unsigned int i;
    PyObject *svcdict;
    ipvs_service_t svc;
    ipvs_service_entry_t *svc_entry;
    struct ip_vs_get_dests *get;

    PyArg_ParseTuple(args, "O", &svcdict);
    pyipvs_dict_to_service(svcdict, &svc);
    svc_entry = ipvs_get_service(svc.fwmark, svc.protocol, svc.addr, svc.port);
    get = ipvs_get_dests(svc_entry);

    if (!get) return PyTuple_New(0);

    PyObject *dests_tuple = PyTuple_New(get->num_dests);

    for (i = 0; i < get->num_dests; i++) {
        PyTuple_SetItem(dests_tuple, i, pyipvs_dest_to_dict(&get->entrytable[i]));
    }

    free(svc_entry);
    free(get);

    return dests_tuple;
}


static PyObject*
pyipvs_set_timeout(PyObject *self, PyObject *args) {
    ipvs_timeout_t timeout;
    PyObject *timeoutdict;

    PyArg_ParseTuple(args, "O", &timeoutdict);

    pyipvs_dict_to_timeout(timeoutdict, &timeout);

    return Py_BuildValue("i", ipvs_set_timeout(&timeout));
}


static PyObject*
pyipvs_get_timeout(PyObject *self, PyObject *args) {
    ipvs_timeout_t *timeout;
    PyObject *timeoutdict;

    timeout = ipvs_get_timeout();
    timeoutdict = pyipvs_timeout_to_dict(timeout);

    free(timeout);

    return timeoutdict;
}


static PyObject*
pyipvs_close(PyObject *self, PyObject *args) {
    ipvs_close();
    Py_RETURN_NONE;
}


static PyObject*
pyipvs_num_services(PyObject *self, PyObject *args) {
    struct ip_vs_get_services *get = ipvs_get_services();
    if (!get) return Py_BuildValue("i", 0);
    return Py_BuildValue("i", get->num_services);
}


static PyMethodDef ipvs_methods[] = {
    {"init", pyipvs_init, METH_NOARGS, "ipvs_init"},
    {"getinfo", pyipvs_getinfo, METH_NOARGS, "ipvs_getinfo"},
    {"flush", pyipvs_flush, METH_NOARGS, "ipvs_flush"},
    {"version", pyipvs_version, METH_NOARGS, "ipvs_version"},
    {"close", pyipvs_close, METH_NOARGS, "ipvs_close"},
    {"num_services", pyipvs_num_services, METH_NOARGS, "ipvs_info.num_services"},
    {"add_service", pyipvs_add_service, METH_VARARGS, "ipvs_add_service"},
    {"update_service", pyipvs_update_service, METH_VARARGS, "ipvs_update_service"},
    {"del_service", pyipvs_del_service, METH_VARARGS, "ipvs_del_service"},
    {"zero_service", pyipvs_zero_service, METH_VARARGS, "ipvs_zero_service"},
    {"get_services", pyipvs_get_services, METH_NOARGS, "ipvs_get_services"},
    {"get_service", pyipvs_get_service, METH_VARARGS, "ipvs_get_service"},
    {"add_dest", pyipvs_add_dest, METH_VARARGS, "ipvs_add_dest"},
    {"update_dest", pyipvs_update_dest, METH_VARARGS, "ipvs_update_dest"},
    {"del_dest", pyipvs_del_dest, METH_VARARGS, "ipvs_del_dest"},
    {"get_dests", pyipvs_get_dests, METH_VARARGS, "ipvs_get_dests"},
    {"start_daemon", pyipvs_start_daemon, METH_VARARGS, "ipvs_start_daemon"},
    {"stop_daemon", pyipvs_stop_daemon, METH_VARARGS, "ipvs_stop_daemon"},
    {"get_daemon", pyipvs_get_daemon, METH_NOARGS, "ipvs_get_daemon"},
    {"get_timeout", pyipvs_get_timeout, METH_NOARGS, "ipvs_get_timeout"},
    {"set_timeout", pyipvs_set_timeout, METH_VARARGS, "ipvs_set_timeout"},
    {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC
init_ipvs(void) {
    Py_InitModule("_ipvs", ipvs_methods);
}
