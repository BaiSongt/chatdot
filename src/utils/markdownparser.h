#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include <QString>
#include <QRegularExpression>

/**
 * @brief 简单的Markdown解析器类
 * 
 * 这个类提供了基本的Markdown到HTML的转换功能，支持常用的Markdown语法
 */
class MarkdownParser
{
public:
    /**
     * @brief 将Markdown文本转换为HTML
     * @param markdown Markdown格式的文本
     * @return 转换后的HTML文本
     */
    static QString toHtml(const QString& markdown) {
        QString html = markdown;
        
        // 转义HTML特殊字符，但保留Markdown语法
        html = html.toHtmlEscaped();
        
        // 解析代码块 ```code```
        QRegularExpression codeBlockRegex("```([\\s\\S]*?)```");
        QRegularExpressionMatchIterator i = codeBlockRegex.globalMatch(html);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString codeBlock = match.captured(1);
            QString htmlCodeBlock = QString("<pre style=\"background-color: #f5f5f5; padding: 10px; border-radius: 5px; font-family: monospace; overflow-x: auto;\"><code>%1</code></pre>").arg(codeBlock);
            html.replace(match.captured(0), htmlCodeBlock);
        }
        
        // 解析行内代码 `code`
        QRegularExpression inlineCodeRegex("`([^`]*?)`");
        i = inlineCodeRegex.globalMatch(html);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString code = match.captured(1);
            QString htmlCode = QString("<code style=\"background-color: #f0f0f0; padding: 2px 4px; border-radius: 3px; font-family: monospace;\">%1</code>").arg(code);
            html.replace(match.captured(0), htmlCode);
        }
        
        // 解析水平线 ---
        QRegularExpression hrRegex("^-{3,}$", QRegularExpression::MultilineOption);
        html.replace(hrRegex, "<hr style=\"border: 1px solid #ddd; margin: 10px 0;\">");
        
        // 解析引用 > text
        QRegularExpression quoteRegex("^&gt; (.*)$", QRegularExpression::MultilineOption);
        html.replace(quoteRegex, "<blockquote style=\"border-left: 4px solid #ddd; padding-left: 10px; color: #666; margin: 10px 0;\">\\1</blockquote>");
        
        // 解析标题 # Heading
        QRegularExpression h1Regex("^# (.*)$", QRegularExpression::MultilineOption);
        html.replace(h1Regex, "<h1 style=\"font-size: 1.8em; margin: 15px 0;\">\\1</h1>");
        
        QRegularExpression h2Regex("^## (.*)$", QRegularExpression::MultilineOption);
        html.replace(h2Regex, "<h2 style=\"font-size: 1.5em; margin: 12px 0;\">\\1</h2>");
        
        QRegularExpression h3Regex("^### (.*)$", QRegularExpression::MultilineOption);
        html.replace(h3Regex, "<h3 style=\"font-size: 1.3em; margin: 10px 0;\">\\1</h3>");
        
        // 解析粗体 **text**
        QRegularExpression boldRegex("\\*\\*(.*?)\\*\\*");
        html.replace(boldRegex, "<strong>\\1</strong>");
        
        // 解析斜体 *text*
        QRegularExpression italicRegex("\\*([^\\*]*?)\\*");
        html.replace(italicRegex, "<em>\\1</em>");
        
        // 解析删除线 ~~text~~
        QRegularExpression strikeRegex("~~(.*?)~~");
        html.replace(strikeRegex, "<del>\\1</del>");
        
        // 解析链接 [text](url)
        QRegularExpression linkRegex("\\[(.*?)\\]\\((.*?)\\)");
        html.replace(linkRegex, "<a href=\"\\2\" style=\"color: #0366d6; text-decoration: none;\">\\1</a>");
        
        // 处理无序列表
        // 先将每个列表项转换为HTML列表项
        QRegularExpression ulItemRegex("^- (.*)$", QRegularExpression::MultilineOption);
        html.replace(ulItemRegex, "<li style=\"margin: 5px 0;\">\\1</li>");
        
        // 处理有序列表
        QRegularExpression olItemRegex("^\\d+\\. (.*)$", QRegularExpression::MultilineOption);
        html.replace(olItemRegex, "<li style=\"margin: 5px 0;\">\\1</li>");
        
        // 将连续的列表项包装在适当的列表标签中
        // 注意：这是一个简化处理，实际上需要更复杂的逻辑来处理嵌套列表
        QRegularExpression consecutiveListItems("(<li.*?>.*?</li>)(<li.*?>.*?</li>)+");
        i = consecutiveListItems.globalMatch(html);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString listItems = match.captured(0);
            QString wrappedList = QString("<ul style=\"padding-left: 20px; margin: 10px 0;\">%1</ul>").arg(listItems);
            html.replace(listItems, wrappedList);
        }
        
        // 将换行符转换为<br>
        html.replace("\n", "<br>");
        
        return html;
    }
};

#endif // MARKDOWNPARSER_H
