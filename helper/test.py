import pprint as _pprint

import ipvs as _ipvs


def make_service(**kwargs):
    defaults = {
        'port': 90,
        'address': '192.168.53.1',
        'protocol': 'TCP',
        'scheduler': 'wlc',
        'u_threshold': 10,
        'l_threshold': 20,
    }
    return dict(defaults, **kwargs)


def make_dest(**kwargs):
    defaults = {
        'address': '192.168.53.2',
        'port': 90,
        'weight': 25,
    }
    return dict(defaults, **kwargs)


_ipvs.init()
_ipvs.add_service(make_service())
_ipvs.add_dest(make_service(), make_dest())
_pprint.pprint(_ipvs.get_services())
_pprint.pprint(_ipvs.num_services())
_ipvs.close()
