#include "custommarkdownhighlighter.h"

CustomMarkdownHighlighter::CustomMarkdownHighlighter(QTextDocument* parent, HighlightingOptions highlightingOptions)
    : MarkdownHighlighter(parent, highlightingOptions) {
    setListsColor(QColor(35, 131, 226));
    _formats[static_cast<HighlighterState>(HighlighterState::HorizontalRuler)].clearBackground();
}

void CustomMarkdownHighlighter::setHeaderColors(const QColor& color) {
    for (int i = HighlighterState::H1; i <= HighlighterState::H6; ++i) {
        _formats[static_cast<HighlighterState>(i)].setForeground(color);
    }
}

void CustomMarkdownHighlighter::setFontSize(qreal fontSize) {
    for (auto& format : _formats) {
        format.setFontPointSize(fontSize);
    }

    for (int i = HighlighterState::H1; i <= HighlighterState::H6; ++i) {
        const qreal size = fontSize * (1.6 - (i - HighlighterState::H1) * 0.1);
        _formats[static_cast<HighlighterState>(i)].setFontPointSize(size);
    }

    _formats[static_cast<HighlighterState>(HighlighterState::InlineCodeBlock)].setFontPointSize(fontSize - 4);
}

void CustomMarkdownHighlighter::setListsColor(const QColor& color) {
    _formats[static_cast<HighlighterState>(HighlighterState::CheckBoxUnChecked)].setForeground(color);
    _formats[static_cast<HighlighterState>(HighlighterState::CheckBoxChecked)].setForeground(QColor(90, 113, 140));
    _formats[static_cast<HighlighterState>(HighlighterState::List)].setForeground(color);
    _formats[static_cast<HighlighterState>(HighlighterState::BlockQuote)].setForeground(color);
}

void CustomMarkdownHighlighter::setTheme(Theme::Value theme, const QColor& textColor, qreal fontSize) {
    setHeaderColors(textColor);
    setFontSize(fontSize);

    auto& inlineCode = _formats[static_cast<HighlighterState>(HighlighterState::InlineCodeBlock)];
    switch (theme) {
        case Theme::Dark:
            inlineCode.setBackground(QColor(52, 57, 66));
            inlineCode.setForeground(QColor(230, 237, 243));
            break;
        case Theme::Sepia:
        case Theme::Light:
        default:
            inlineCode.setBackground(QColor(239, 241, 243));
            inlineCode.setForeground(QColor(42, 46, 51));
            break;
    }
}
