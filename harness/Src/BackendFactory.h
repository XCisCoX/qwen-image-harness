#ifndef BACKENDFACTORY_H
#define BACKENDFACTORY_H

class Backend;
class QObject;

/*!
 * The single place where the UI meets the real service.
 * See BackendFactory.cpp for the soldering instructions.
 */
Backend *createBackend(QObject *parent);

#endif // BACKENDFACTORY_H
