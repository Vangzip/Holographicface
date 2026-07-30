#pragma once

#include <QStringList>

// Returns true when this process handled an isolated child scenario and the
// caller should exit without running the rest of the suite.
bool runImc60gSafetyTests(const QStringList& arguments);
