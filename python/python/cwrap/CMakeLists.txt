set(PYTHON_SOURCES
    __init__.py
    basecclass.py
    basecenum.py
    basecvalue.py
    cclass.py
    cenum.py
    cfile.py
    clib.py
    cnamespace.py
    cwrap.py
    metacwrap.py
    prototype.py
)

add_python_package("cwrap"  ${PYTHON_INSTALL_PREFIX}/cwrap "${PYTHON_SOURCES}" True)
