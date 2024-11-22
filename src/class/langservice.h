#ifndef LANGSERVICE_H
#define LANGSERVICE_H

#include "qlanguagefactory.h"
#include "qsnippetbinding.h"

#include "angelscript.h"

#include <QObject>

class LangService : QObject {
    Q_OBJECT

public:
    static LangService &instance();

    void init(asIScriptEngine *engine);

    QSnippetBinding *snippetBinding() const;

    QLanguageFactory *languageFactory() const;

    void applyLanguageSerivce(QEditor *editor);

    QSnippetManager *snippetManager() const;

private:
    LangService();
    ~LangService();

private:
    QSnippetBinding *m_snipbind = nullptr;
    QLanguageFactory *m_language = nullptr;
    QSnippetManager *m_snippetManager = nullptr;

    Q_DISABLE_COPY_MOVE(LangService)
};

#endif // LANGSERVICE_H
