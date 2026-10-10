// JSONC document model that keeps comments, key order and formatting.
//
// Waybar configs are JSONC: people annotate them heavily. Re-serialising the
// parsed tree with QJsonDocument throws all of that away, so instead every node
// remembers the byte span it occupies in the original text. Untouched subtrees
// are copied verbatim; only values that actually changed are re-rendered.
#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

#include <memory>

namespace ws {

enum class JType { Scalar, Object, Array };

struct JsonNode;
using NodePtr = std::shared_ptr<JsonNode>;

struct JMember {
  QString key;
  int keyStart = 0;
  int keyEnd = 0;
  NodePtr value;
  bool removed = false;
  bool isNew = false;
};

struct JsonNode {
  JType type = JType::Scalar;
  QJsonValue value;             // scalars only
  int start = -1;               // span in the source text, -1 when synthesised
  int end = -1;
  QList<JMember> members;       // Object
  QList<NodePtr> items;         // Array
  QList<NodePtr> newItems;      // Array, appended after the existing ones
  bool dirty = false;           // scalar value replaced
  bool removed = false;         // array element dropped
};

class JsoncDoc {
 public:
  bool parse(const QString &text, QString *error);
  const QString &text() const { return src_; }
  NodePtr root() const { return root_; }
  QString render() const;

  static NodePtr makeScalar(const QJsonValue &v);
  static NodePtr makeObject();
  static NodePtr makeArray();

  // Replaces the root node; the new node may reuse nodes taken from this
  // document, whose original text is still available for untouched subtrees.
  void setRoot(const NodePtr &root) { root_ = root; }
  NodePtr makeRootArray(const QList<NodePtr> &items);

  static NodePtr member(const NodePtr &obj, const QString &key);
  static NodePtr ensureMember(const NodePtr &obj, const QString &key);
  static bool removeMember(const NodePtr &obj, const QString &key);
  static void setValue(const NodePtr &obj, const QString &key, const QJsonValue &v);
  static void setStringList(const NodePtr &arr, const QStringList &vals);
  static QStringList stringList(const NodePtr &arr);

  static QJsonValue toJson(const NodePtr &n);
  static QJsonObject toObject(const NodePtr &n);

 private:
  QString src_;
  NodePtr root_;
};

// Renders a QJsonValue as compact JSON text (strings keep their quotes).
QString jsonCompact(const QJsonValue &v);

}  // namespace ws
