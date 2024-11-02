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

#include "qasparser.h"

#include "AngelScript/sdk/angelscript/source/as_builder.h"
#include "AngelScript/sdk/angelscript/source/as_parser.h"
#include "codemodel/qcodemodel.h"
#include "codemodel/qcodenode.h"

#include <QDebug>

/*
 * Completion only support: Internal classes, enums and methods,
 *                          Standard libraries and included headers,
 *                          global classes, enums and methods in opened codes
 *
 * If you want to make it more powerful, PR will be welcomed.
 */

QAsParser::QAsParser(asIScriptEngine *engine) : asBuilder(), _engine(engine) {
    addClassCompletion(engine);
    addEnumCompletion(engine);
    addGlobalFunctionCompletion(engine);
}

QAsParser::~QAsParser() {
    qDeleteAll(_headerNodes);
    _headerNodes.clear();
}

QByteArray QAsParser::getFnParamDeclString(asIScriptFunction *fn,
                                           bool includeNamespace,
                                           bool includeParamNames) {
    auto fun = dynamic_cast<asCScriptFunction *>(fn);
    if (fun == nullptr) {
        return {};
    }

    asCString str;

    auto &parameterTypes = fun->parameterTypes;
    auto &parameterNames = fun->parameterNames;
    auto &nameSpace = fun->nameSpace;
    auto &inOutFlags = fun->inOutFlags;
    auto &defaultArgs = fun->defaultArgs;

    if (parameterTypes.GetLength() > 0) {
        asUINT n;
        for (n = 0; n < parameterTypes.GetLength() - 1; n++) {
            str += parameterTypes[n].Format(nameSpace, includeNamespace);
            if (parameterTypes[n].IsReference() && inOutFlags.GetLength() > n) {
                if (inOutFlags[n] == asTM_INREF)
                    str += "in";
                else if (inOutFlags[n] == asTM_OUTREF)
                    str += "out";
                else if (inOutFlags[n] == asTM_INOUTREF)
                    str += "inout";
            }

            if (includeParamNames && n < parameterNames.GetLength() &&
                parameterNames[n].GetLength() != 0) {
                str += " ";
                str += parameterNames[n];
            }

            if (defaultArgs.GetLength() > n && defaultArgs[n]) {
                asCString tmp;
                tmp.Format(" = %s", defaultArgs[n]->AddressOf());
                str += tmp;
            }

            str += ", ";
        }

        // Add the last parameter
        str += parameterTypes[n].Format(nameSpace, includeNamespace);
        if (parameterTypes[n].IsReference() && inOutFlags.GetLength() > n) {
            if (inOutFlags[n] == asTM_INREF)
                str += "in";
            else if (inOutFlags[n] == asTM_OUTREF)
                str += "out";
            else if (inOutFlags[n] == asTM_INOUTREF)
                str += "inout";
        }

        if (includeParamNames && n < parameterNames.GetLength() &&
            parameterNames[n].GetLength() != 0) {
            str += " ";
            str += parameterNames[n];
        }

        if (defaultArgs.GetLength() > n && defaultArgs[n]) {
            asCString tmp;
            tmp.Format(" = %s", defaultArgs[n]->AddressOf());
            str += tmp;
        }
    }

    return QByteArray(str.AddressOf(), str.GetLength());
}

QByteArray QAsParser::getFnRealName(asIScriptFunction *fn) {
    auto fun = dynamic_cast<asCScriptFunction *>(fn);
    if (fun == nullptr) {
        return {};
    }

    asCString str;
    asCString name = fun->GetName();

    if (name.GetLength() == 0)
        str += "_unnamed_function_";
    else if (name.SubString(0, 4) == "$beh" && name.GetLength() == 5) {
        if (name[4] == '0' + asBEHAVE_CONSTRUCT)
            str += fun->objectType->name;
        else if (name[4] == '0' + asBEHAVE_FACTORY)
            str += fun->returnType.GetTypeInfo()->name;
        else if (name[4] == '0' + asBEHAVE_DESTRUCT)
            str += "~" + fun->objectType->name;
        else
            str += name;
    } else {
        str = name;
    }

    return QByteArray(str.AddressOf(), str.GetLength());
}

QByteArray QAsParser::getFnRetTypeString(asIScriptFunction *fn,
                                         bool includeNamespace) {
    auto fun = dynamic_cast<asCScriptFunction *>(fn);
    if (fun == nullptr) {
        return {};
    }

    auto &returnType = fun->returnType;
    auto &objectType = fun->objectType;
    auto &name = fun->name;
    auto &nameSpace = fun->nameSpace;

    if (!(returnType.GetTokenType() == ttVoid && objectType &&
          (name == objectType->name ||
           (name.GetLength() > 0 && name[0] == '~') || name == "$beh0" ||
           name == "$beh2"))) {
        auto str = returnType.Format(nameSpace, includeNamespace);
        return QByteArray(str.AddressOf(), str.GetLength());
    }

    return {};
}

bool QAsParser::parse(const QString &filename) {
    if (StartNewModule(_engine, "as_parser") != 0) {
        return false;
    }

    auto ret = LoadScriptSection(filename);
    if (ret != 0) {
        return false;
    }

    ClearAll();

    auto mod = dynamic_cast<asCModule *>(GetModule());
    Q_ASSERT(mod);
    asCParser parser(mod->m_builder);

    m_code.reset(new asCScriptCode);
    m_code->SetCode("as_parser", modifiedScript.data(), true);

    parser.ParseScript(m_code.get());

    auto pnodes = parser.GetScriptNode();

    QList<QCodeNode *> qnodes;

    // do {
    //     auto node = asNode2CodeNode(pnodes);

    //     auto p = pnodes->firstChild;
    //     while (p) {
    //         auto cnode = asNode2CodeNode(pnodes);
    //         cnode->parent = node;
    //         node->children.append(cnode);

    //         p = pnodes->next;
    //     }

    //     qnodes.append(node);
    //     pnodes = pnodes->next;
    // } while (pnodes != nullptr);

    return true;
}

bool QAsParser::parse(const QString &code, const QString &section) {
    return ProcessScriptSection(code.toUtf8(), code.length(), section, 0);
}

const QList<QCodeNode *> &QAsParser::headerNodes() const {
    return _headerNodes;
}

void QAsParser::addGlobalFunctionCompletion(asIScriptEngine *engine) {
    Q_ASSERT(engine);

    QHash<QByteArray, QList<FnInfo>> _maps;

    for (asUINT i = 0; i < engine->GetGlobalFunctionCount(); ++i) {
        auto fn = engine->GetGlobalFunctionByIndex(i);

        FnInfo fnInfo;
        auto ns = fn->GetNamespace();
        fnInfo.retType = getFnRetTypeString(fn, true);
        fnInfo.fnName = fn->GetName();
        fnInfo.params = getFnParamDeclString(fn, false, true);
        fnInfo.isConst = fn->IsReadOnly();

        _maps[ns] << fnInfo;
    }

    for (auto p = _maps.keyValueBegin(); p != _maps.keyValueEnd(); p++) {
        auto node = new QCodeNode;
        _headerNodes << node;
        if (p->first.isEmpty()) {
            node->setNodeType(QCodeNode::Group);
        } else {
            node->setNodeType(QCodeNode::Namespace);
        }
        node->setRole(QCodeNode::Name, p->first);

        auto pnodes = &node->children();
        auto nodeParent = node;

        for (auto &fn : p->second) {
            auto node = new QCodeNode;
            node->setNodeType(QCodeNode::Function);
            node->setRole(QCodeNode::Return, fn.retType);
            node->setRole(QCodeNode::Name, fn.fnName);
            node->setRole(QCodeNode::Arguments, fn.params);
            QByteArray qualifiers;
            if (fn.isConst) {
                qualifiers.setNum(QCodeNode::QUALIFIER_CONST);
            }
            node->setRole(QCodeNode::Qualifiers, qualifiers);
            node->setParent(nodeParent);
            pnodes->append(node);
        }
    }
}

void QAsParser::addEnumCompletion(asIScriptEngine *engine) {
    Q_ASSERT(engine);

    QHash<QByteArray, QList<EnumInfo>> _maps;

    for (asUINT i = 0; i < engine->GetEnumCount(); ++i) {
        auto etype = engine->GetEnumByIndex(i);
        etype->AddRef();

        EnumInfo einfo;
        auto ns = etype->GetNamespace();
        einfo.name = etype->GetName();

        for (asUINT i = 0; i < etype->GetEnumValueCount(); ++i) {
            int v;
            auto e = etype->GetEnumValueByIndex(i, &v);
            einfo.enums.append(qMakePair(QByteArray(e), v));
        }

        etype->Release();

        _maps[ns] << einfo;
    }

    for (auto p = _maps.keyValueBegin(); p != _maps.keyValueEnd(); p++) {
        auto node = new QCodeNode;
        _headerNodes << node;
        if (p->first.isEmpty()) {
            node->setNodeType(QCodeNode::Group);
        } else {
            node->setNodeType(QCodeNode::Namespace);
        }
        node->setRole(QCodeNode::Name, p->first);

        auto pnodes = &node->children();
        auto nodeParent = node;
        for (auto &e : p->second) {
            auto node = new QCodeNode;
            node->setNodeType(QCodeNode::Enum);
            node->setRole(QCodeNode::Name, e.name);
            node->setParent(nodeParent);
            pnodes->append(node);

            auto enode = new QCodeNode;
            _headerNodes << enode;
            enode->setNodeType(QCodeNode::Enum);
            enode->setRole(QCodeNode::Name, e.name);
            for (auto &ev : e.enums) {
                auto node = new QCodeNode;
                node->setNodeType(QCodeNode::Enumerator);
                node->setRole(QCodeNode::Name, ev.first);
                QByteArray value;
                value.setNum(ev.second);
                node->setRole(QCodeNode::Value, value);
                node->setParent(enode);
                enode->children().append(node);
            }
        }
    }
}

void QAsParser::addClassCompletion(asIScriptEngine *engine) {
    auto eng = dynamic_cast<asCScriptEngine *>(engine);
    Q_ASSERT(eng);

    QHash<QByteArray, QList<ClassInfo>> _maps;

    for (asUINT i = 0; i < engine->GetObjectTypeCount(); ++i) {
        auto obj = eng->registeredObjTypes[i];
        obj->AddRef();

        ClassInfo cls;
        cls.name = obj->GetName();
        auto ns = obj->GetNamespace();

        for (asUINT i = 0; i < obj->GetBehaviourCount(); ++i) {
            asEBehaviours bv;
            auto b = obj->GetBehaviourByIndex(i, &bv);

            switch (bv) {
            case asBEHAVE_CONSTRUCT:
            case asBEHAVE_DESTRUCT: {
                // only these are supported
                b->AddRef();
                FnInfo fn;
                fn.fnName = getFnRealName(b);
                fn.params = getFnParamDeclString(b, false, true);
                fn.isConst = b->IsReadOnly();
                cls.methods << fn;
                b->Release();
            }
            default:
                continue;
            }
        }

        for (asUINT i = 0; i < obj->GetMethodCount(); ++i) {
            auto m = obj->GetMethodByIndex(i, true);

            m->AddRef();
            FnInfo fn;
            fn.retType = getFnRetTypeString(m, true);
            fn.fnName = getFnRealName(m);
            fn.params = getFnParamDeclString(m, false, true);
            fn.isConst = m->IsReadOnly();
            cls.methods << fn;
            m->Release();
        }

        for (asUINT i = 0; i < obj->GetPropertyCount(); ++i) {
            auto p = obj->properties[i];

            PropertyInfo pi;
            pi.name = QByteArray(p->name.AddressOf(), p->name.GetLength());
            auto tn = p->type.Format(obj->nameSpace);
            pi.type = QByteArray(tn.AddressOf(), tn.GetLength());
            pi.isPrivate = pi.isPrivate;
            pi.isProtected = pi.isProtected;
            pi.isRef = pi.isRef;

            cls.properties << pi;
        }

        obj->Release();

        _maps[ns] << cls;
    }

    for (auto p = _maps.keyValueBegin(); p != _maps.keyValueEnd(); p++) {
        auto node = new QCodeNode;
        _headerNodes << node;
        if (p->first.isEmpty()) {
            node->setNodeType(QCodeNode::Group);
        } else {
            node->setNodeType(QCodeNode::Namespace);
        }
        node->setRole(QCodeNode::Name, p->first);

        // TODO
    }
}

QList<QCodeNode *> QAsParser::codeNodes() const { return _nodes; }
