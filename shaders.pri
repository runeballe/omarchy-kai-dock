# Compiles the GLSL shaders to Qt's .qsb format next to their sources, before
# rcc packs them; kai.pro and the tests both include this.
SHADERS = $$PWD/src/shaders/wave.vert $$PWD/src/shaders/wave.frag
qsb.input = SHADERS
qsb.output = $$PWD/src/shaders/${QMAKE_FILE_IN_BASE}${QMAKE_FILE_EXT}.qsb
qsb.commands = $$[QT_HOST_BINS]/qsb --qt6 -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
qsb.CONFIG += no_link target_predeps
qsb.variable_out = QSB_FILES
QMAKE_EXTRA_COMPILERS += qsb
