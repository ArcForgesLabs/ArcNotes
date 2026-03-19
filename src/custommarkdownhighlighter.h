#ifndef CUSTOMMARKDOWNHIGHLIGHTER_H
#define CUSTOMMARKDOWNHIGHLIGHTER_H

#include <QColor>

#include "editorsettingsoptions.h"
#include "markdownhighlighter.h"

class CustomMarkdownHighlighter : public MarkdownHighlighter {
public:
    explicit CustomMarkdownHighlighter(QTextDocument* parent,
                                       HighlightingOptions highlightingOptions = HighlightingOption::None);
    ~CustomMarkdownHighlighter() override = default;

    void setFontSize(qreal fontSize);
    void setHeaderColors(const QColor& color);
    void setListsColor(const QColor& color);
    void setTheme(Theme::Value theme, const QColor& textColor, qreal fontSize);
};

#endif  // CUSTOMMARKDOWNHIGHLIGHTER_H
