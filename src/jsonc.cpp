#include "jsonc.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>

namespace ws {
  namespace {

    inline bool isValueTerminator(QChar c) {
      return c.isSpace() || c == ',' || c == ']' || c == '}' || c == ':';
    }

    struct Parser {
      const QString &s;
      int i = 0;
      QString err;

      explicit Parser(const QString &t) : s(t) {}

      void fail(const QString &m) {
        if (err.isEmpty()) err = m;
      }

      void skipTrivia() {
        while (i < s.size()) {
          const QChar c = s.at(i);
          if (c.isSpace()) {
            ++i;
            continue;
          }
          if (c == '/' && i + 1 < s.size()) {
            if (s.at(i + 1) == '/') {
              while (i < s.size() && s.at(i) != '\n') ++i;
              continue;
            }
            if (s.at(i + 1) == '*') {
              i += 2;
              while (i + 1 < s.size() && !(s.at(i) == '*' && s.at(i + 1) == '/')) ++i;
              if (i + 1 < s.size())
                i += 2;
              else {
                i = s.size();
                fail("unterminated block comment");
              }
              continue;
            }
          }
          break;
        }
      }

      // Scans a double-quoted string starting at s[i]; leaves i past the closing quote.
      void scanString() {
        ++i;
        while (i < s.size()) {
          const QChar c = s.at(i);
          if (c == '\\') {
            i += 2;
            continue;
          }
          if (c == '"') {
            ++i;
            return;
          }
          ++i;
        }
        fail("unterminated string");
      }

      NodePtr scalar() {
        skipTrivia();
        if (i >= s.size()) {
          fail("unexpected end of input");
          return nullptr;
        }
        const int start = i;
        if (s.at(i) == '"') {
          scanString();
        } else {
          while (i < s.size() && !isValueTerminator(s.at(i))) ++i;
        }
        if (i == start) {
          fail(QStringLiteral("unexpected character '%1'").arg(s.at(i)));
          return nullptr;
        }
        const QString token = s.mid(start, i - start);
        QJsonParseError pe;
        const QJsonDocument d = QJsonDocument::fromJson(("[" + token + "]").toUtf8(), &pe);
        if (pe.error != QJsonParseError::NoError || !d.isArray() || d.array().isEmpty()) {
          fail("invalid JSON value '" + token + "'");
          return nullptr;
        }
        auto n = std::make_shared<JsonNode>();
        n->type = JType::Scalar;
        n->start = start;
        n->end = i;
        n->value = d.array().at(0);
        return n;
      }

      NodePtr object() {
        auto n = std::make_shared<JsonNode>();
        n->type = JType::Object;
        n->start = i;
        ++i;  // '{'
        skipTrivia();
        if (i < s.size() && s.at(i) == '}') {
          n->end = i + 1;
          ++i;
          return n;
        }
        while (true) {
          skipTrivia();
          if (i >= s.size() || s.at(i) != '"') {
            fail("expected a quoted key");
            return nullptr;
          }
          JMember m;
          m.keyStart = i;
          scanString();
          m.keyEnd = i;
          QJsonParseError pe;
          const QJsonDocument kd = QJsonDocument::fromJson(("[" + s.mid(m.keyStart, m.keyEnd - m.keyStart) + "]").toUtf8(), &pe);
          if (pe.error != QJsonParseError::NoError || !kd.isArray() || kd.array().isEmpty()) {
            fail("invalid object key");
            return nullptr;
          }
          m.key = kd.array().at(0).toString();
          skipTrivia();
          if (i >= s.size() || s.at(i) != ':') {
            fail("expected ':' after key \"" + m.key + "\"");
            return nullptr;
          }
          ++i;
          m.value = scalar();
          if (!m.value) return nullptr;
          n->members.append(m);
          skipTrivia();
          if (i < s.size() && s.at(i) == ',') {
            ++i;
            skipTrivia();
            if (i < s.size() && s.at(i) == '}') {  // tolerate trailing commas
              n->end = i + 1;
              ++i;
              return n;
            }
            continue;
          }
          if (i < s.size() && s.at(i) == '}') {
            n->end = i + 1;
            ++i;
            return n;
          }
          fail("expected ',' or '}'");
          return nullptr;
        }
      }

      NodePtr array() {
        auto n = std::make_shared<JsonNode>();
        n->type = JType::Array;
        n->start = i;
        ++i;  // '['
        skipTrivia();
        if (i < s.size() && s.at(i) == ']') {
          n->end = i + 1;
          ++i;
          return n;
        }
        while (true) {
          auto item = scalar();
          if (!item) return nullptr;
          n->items.append(item);
          skipTrivia();
          if (i < s.size() && s.at(i) == ',') {
            ++i;
            skipTrivia();
            if (i < s.size() && s.at(i) == ']') {
              n->end = i + 1;
              ++i;
              return n;
            }
            continue;
          }
          if (i < s.size() && s.at(i) == ']') {
            n->end = i + 1;
            ++i;
            return n;
          }
          fail("expected ',' or ']'");
          return nullptr;
        }
      }

      NodePtr value() {
        skipTrivia();
        if (i >= s.size()) {
          fail("unexpected end of input");
          return nullptr;
        }
        const QChar c = s.at(i);
        if (c == '{') return object();
        if (c == '[') return array();
        return scalar();
      }
    };

    bool isSubtreeDirty(const NodePtr &n) {
      if (!n) return false;
      if (n->type == JType::Scalar) return n->dirty || n->start < 0;
      if (n->type == JType::Object) {
        for (const JMember &m : n->members)
          if (m.removed || m.isNew || isSubtreeDirty(m.value)) return true;
          return false;
      }
      for (const NodePtr &it : n->items)
        if (it->removed || isSubtreeDirty(it)) return true;
        for (const NodePtr &it : n->newItems)
          if (isSubtreeDirty(it)) return true;
          return false;
    }

    struct Renderer {
      const QString &src;

      explicit Renderer(const QString &s) : src(s) {}

      static QString indentationAt(const QString &text, int pos) {
        const int ls = text.lastIndexOf('\n', qMax(0, pos - 1)) + 1;
        QString out;
        for (int k = ls; k < text.size() && (text.at(k) == ' ' || text.at(k) == '\t'); ++k) out += text.at(k);
        return out;
      }

      // Separator between two emitted entries: keeps the original whitespace, or
      // synthesises a newline when entries were removed from in between.
      QString separator(int prevEnd, int nextStart, const QString &fallbackIndent, bool leading) {
        if (prevEnd < 0 || nextStart < prevEnd) return QString();
        const QString gap = src.mid(prevEnd, nextStart - prevEnd);
        const int comma = gap.indexOf(',');
        if (comma < 0) {
          // No comma in between: this is either the very first entry or the gap was
          // already consumed. Anything but plain whitespace means removed content.
          for (const QChar &c : gap)
            if (!c.isSpace()) return fallbackIndent;
            return leading ? gap : QString();
        }
        const QString head = gap.left(comma);
        const QString tail = gap.mid(comma + 1);
        for (const QChar &c : head)
          if (!c.isSpace()) return fallbackIndent;  // removed entries lived here
          return tail;
      }

      static QString stripTrailingComma(QString s) {
        QRegularExpression re(QStringLiteral(",(\\s*)$"));
        return s.replace(re, QStringLiteral("\\1"));
      }

      QString tailAfter(int emittedEnd, const NodePtr &n, int depth, bool any) {
        if (emittedEnd >= 0 && n->end > emittedEnd)
          return src.mid(emittedEnd, n->end - emittedEnd);
        if (n->start >= 0) return QStringLiteral("\n") + indentationAt(src, n->end - 1);
        return any ? QStringLiteral("\n") + QString(depth * 2, QLatin1Char(' ')) : QString();
      }

      QString emitNode(const NodePtr &n, int depth) {
        if (!n) return QString();
        if (n->type == JType::Scalar) {
          if (n->dirty || n->start < 0) return jsonCompact(n->value);
          return src.mid(n->start, n->end - n->start);
        }
        if (!isSubtreeDirty(n) && n->start >= 0) return src.mid(n->start, n->end - n->start);
        return n->type == JType::Object ? emitObject(n, depth) : emitArray(n, depth);
      }

      QString emitObject(const NodePtr &n, int depth) {
        const QString itemIndent =
        (n->start >= 0 && !n->members.isEmpty()) ? indentationAt(src, n->members.first().keyStart)
        : QString((depth + 1) * 2, QLatin1Char(' '));
        QString out = QStringLiteral("{");
        int prevEnd = n->start >= 0 ? n->start + 1 : -1;
        int lastFromSource = -1;
        bool any = false;
        for (const JMember &m : n->members) {
          if (m.removed || m.isNew) continue;
          const QString sep = separator(prevEnd, m.keyStart, QStringLiteral("\n") + itemIndent, !any);
          if (any) out += QLatin1Char(',');
          out += sep;
          out += src.mid(m.keyStart, m.keyEnd - m.keyStart);
          out += src.mid(m.keyEnd, m.value->start - m.keyEnd);
          out += emitNode(m.value, depth + 1);
          prevEnd = m.value->end;
          lastFromSource = prevEnd;
          any = true;
        }
        for (const JMember &m : n->members) {
          if (!m.isNew || m.removed) continue;
          if (any) out += QLatin1Char(',');
          out += QStringLiteral("\n") + itemIndent + jsonCompact(QJsonValue(m.key)) +
          QStringLiteral(": ") + emitNode(m.value, depth + 1);
          any = true;
        }
        out += tailAfter(lastFromSource, n, depth, any);
        return out + QLatin1Char('}');
      }

      QString emitArray(const NodePtr &n, int depth) {
        QString itemIndent;
        for (const NodePtr &it : n->items) {
          if (it->removed) continue;
          itemIndent = indentationAt(src, it->start);
          break;
        }
        if (itemIndent.isEmpty()) itemIndent = QString((depth + 1) * 2, QLatin1Char(' '));

        QString out = QStringLiteral("[");
        int prevEnd = n->start >= 0 ? n->start + 1 : -1;
        int lastFromSource = -1;
        bool any = false;
        for (const NodePtr &it : n->items) {
          if (it->removed) continue;
          const QString sep = separator(prevEnd, it->start, QStringLiteral("\n") + itemIndent, !any);
          if (any) out += QLatin1Char(',');
          out += sep;
          out += emitNode(it, depth + 1);
          prevEnd = it->end;
          lastFromSource = prevEnd;
          any = true;
        }
        for (const NodePtr &it : n->newItems) {
          if (any)
            out += QStringLiteral(",\n") + itemIndent;
          else
            out += QStringLiteral("\n") + itemIndent;
          out += emitNode(it, depth + 1);
          any = true;
        }
        out += stripTrailingComma(tailAfter(lastFromSource, n, depth, any));
        return out + QLatin1Char(']');
      }
    };

  }  // namespace

  QString jsonCompact(const QJsonValue &v) {
    const QByteArray raw = QJsonDocument(QJsonArray{v}).toJson(QJsonDocument::Compact);
    const QString s = QString::fromUtf8(raw);
    return s.size() >= 2 ? s.mid(1, s.size() - 2) : s;
  }

  bool JsoncDoc::parse(const QString &text, QString *error) {
    src_ = text;
    root_.reset();
    Parser p(src_);
    root_ = p.value();
    p.skipTrivia();
    if (!root_) {
      if (error) *error = p.err.isEmpty() ? QStringLiteral("empty document") : p.err;
      return false;
    }
    if (p.i < src_.size()) {
      if (error) *error = p.err.isEmpty() ? QStringLiteral("trailing data after the root value") : p.err;
      return false;
    }
    if (error) error->clear();
    return true;
  }

  QString JsoncDoc::render() const {
    if (!root_) return src_;
    Renderer r(src_);
    return r.emitNode(root_, 0);
  }

  NodePtr JsoncDoc::makeScalar(const QJsonValue &v) {
    auto n = std::make_shared<JsonNode>();
    n->type = JType::Scalar;
    n->value = v;
    n->dirty = true;
    return n;
  }

  NodePtr JsoncDoc::makeObject() {
    auto n = std::make_shared<JsonNode>();
    n->type = JType::Object;
    return n;
  }

  NodePtr JsoncDoc::makeRootArray(const QList<NodePtr> &items) {
    auto n = makeArray();
    n->newItems = items;
    return n;
  }

  NodePtr JsoncDoc::makeArray() {
    auto n = std::make_shared<JsonNode>();
    n->type = JType::Array;
    return n;
  }

  NodePtr JsoncDoc::member(const NodePtr &obj, const QString &key) {
    if (!obj || obj->type != JType::Object) return nullptr;
    for (const JMember &m : obj->members)
      if (!m.removed && !m.isNew && m.key == key) return m.value;
      return nullptr;
  }

  NodePtr JsoncDoc::ensureMember(const NodePtr &obj, const QString &key) {
    NodePtr existing = member(obj, key);
    if (existing) return existing;
    if (!obj || obj->type != JType::Object) return nullptr;
    JMember m;
    m.key = key;
    m.value = makeObject();
    m.isNew = true;
    obj->members.append(m);
    return m.value;
  }

  bool JsoncDoc::removeMember(const NodePtr &obj, const QString &key) {
    if (!obj || obj->type != JType::Object) return false;
    bool found = false;
    for (JMember &m : obj->members) {
      if (!m.removed && !m.isNew && m.key == key) {
        m.removed = true;
        found = true;
      }
    }
    return found;
  }

  void JsoncDoc::setValue(const NodePtr &obj, const QString &key, const QJsonValue &v) {
    NodePtr existing = member(obj, key);
    if (existing) {
      if (existing->type != JType::Scalar) return;
      if (existing->value == v) return;
      existing->value = v;
      existing->dirty = true;
      return;
    }
    if (!obj || obj->type != JType::Object) return;
    JMember m;
    m.key = key;
    m.value = makeScalar(v);
    m.isNew = true;
    obj->members.append(m);
  }

  void JsoncDoc::setStringList(const NodePtr &arr, const QStringList &vals) {
    if (!arr || arr->type != JType::Array) return;
    if (stringList(arr) == vals) return;
    for (const NodePtr &it : arr->items) it->removed = true;
    arr->newItems.clear();
    for (const QString &v : vals) arr->newItems.append(makeScalar(QJsonValue(v)));
  }

  QStringList JsoncDoc::stringList(const NodePtr &arr) {
    QStringList out;
    if (!arr || arr->type != JType::Array) return out;
    for (const NodePtr &it : arr->items)
      if (!it->removed && it->type == JType::Scalar) out << it->value.toVariant().toString();
      for (const NodePtr &it : arr->newItems)
        if (it->type == JType::Scalar) out << it->value.toVariant().toString();
        return out;
  }

  QJsonValue JsoncDoc::toJson(const NodePtr &n) {
    if (!n) return QJsonValue(QJsonValue::Undefined);
    switch (n->type) {
      case JType::Scalar:
        return n->value;
      case JType::Array: {
        QJsonArray a;
        for (const NodePtr &it : n->items)
          if (!it->removed) a.append(toJson(it));
          for (const NodePtr &it : n->newItems) a.append(toJson(it));
          return a;
      }
      case JType::Object:
      default: {
        QJsonObject o;
        for (const JMember &m : n->members)
          if (!m.removed && !m.isNew) o.insert(m.key, toJson(m.value));
          return o;
      }
    }
  }

  QJsonObject JsoncDoc::toObject(const NodePtr &n) { return toJson(n).toObject(); }

}  // namespace ws
