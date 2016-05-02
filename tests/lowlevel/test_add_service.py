# coding: utf-8

import pytest as _pytest

import ipvs as _ipvs

pytestmark = _pytest.mark.usefixtures('ipvs')


def test_ipvs_add_service(service):
    result = _ipvs.add_service(service)
    assert result == 0


def test_ipvs_add_service_fails_on_duplicates(service):
    _ipvs.add_service(service)
    result = _ipvs.add_service(service)
    assert result == -1
