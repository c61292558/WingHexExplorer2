/*==============================================================================
** Copyright (C) 2024-2027 WingSummer
**
** This program is free software: you can redistribute it and/or modify it under
** the terms of the GNU Affero General Public License as published by the Free
** Software Foundation, version 3.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
** details.
**
** You should have received a copy of the GNU Affero General Public License
** along with this program. If not, see <https://www.gnu.org/licenses/>.
** =============================================================================
*/

#ifndef _QAS_PARSER_H_
#define _QAS_PARSER_H_

#include "class/asbuilder.h"

#include <QScopedPointer>

class asCScriptCode;
class asCScriptNode;
class QCodeNode;

class QAsParser : protected asBuilder {
public:
    explicit QAsParser(asIScriptEngine *engine);
    virtual ~QAsParser();

private:
    struct FnInfo {
        QByteArray retType;
        QByteArray fnName;
        QByteArray params;
        bool isConst = false;
    };

    struct EnumInfo {
        QByteArray name;
        QList<QPair<QByteArray, int>> enums;
    };

    struct PropertyInfo {
        QByteArray name;
        QByteArray type;
        bool isProtected = false;
        bool isPrivate = false;
        bool isRef = false;
    };

    struct ClassInfo {
        QByteArray name;
        QList<FnInfo> methods;
        QList<PropertyInfo> properties;
    };

private:
    QByteArray getFnParamDeclString(asIScriptFunction *fn,
                                    bool includeNamespace,
                                    bool includeParamNames);

    QByteArray getFnRealName(asIScriptFunction *fn);

    QByteArray getFnRetTypeString(asIScriptFunction *fn, bool includeNamespace);

public:
    bool parse(const QString &filename);
    bool parse(const QString &code, const QString &section);

    QList<QCodeNode *> codeNodes() const;

    const QList<QCodeNode *> &headerNodes() const;

    QList<QCodeNode *> classNodes() const;

private:
    void addGlobalFunctionCompletion(asIScriptEngine *engine);
    void addEnumCompletion(asIScriptEngine *engine);
    void addClassCompletion(asIScriptEngine *engine);

private:
    QCodeNode *newFnCodeNode(const FnInfo &info);

    QCodeNode *newEnumCodeNode(const EnumInfo &info);

private:
    asIScriptEngine *_engine;
    QScopedPointer<asCScriptCode> m_code;
    QList<QCodeNode *> _headerNodes;
    QList<QCodeNode *> _clsNodes;
    QList<QCodeNode *> _nodes;
};

#endif // !_QCPP_PARSER_H_
