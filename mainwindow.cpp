#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>
bool MainWindow::compare(const QPair<QString,QString> &first, const QPair<QString,QString> &second)
{
	QMap<QString,int> lengths;
	lengths["float"] = 4;
	lengths["double"] = 4;
	lengths["char"] = 1;
	lengths["int8_t"] = 1;
	lengths["uint8_t"] = 1;
	lengths["uint8_t_mavlink_version"] = 1;
	lengths["int16_t"] = 2;
	lengths["uint16_t"] = 2;
	lengths["int32_t"] = 4;
	lengths["uint32_t"] = 4;
	lengths["int64_t"] = 8;
	lengths["uint64_t"] = 8;
	QString firststr = first.first;
	QString secondstr = second.first;
	if (firststr.indexOf("[") != -1)
	{
		firststr = firststr.mid(0,firststr.indexOf("["));
	}
	if (secondstr.indexOf("[") != -1)
	{
		secondstr = secondstr.mid(0,secondstr.indexOf("["));
	}
	if (lengths.contains(firststr) && lengths.contains(secondstr))
	{
		return lengths.value(firststr) >= lengths.value(secondstr);
	}
	else
	{
		return false;
	}
	return false;
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{


	ui->setupUi(this);
	QFile common("C:\\Users\\Michael\\code\\mavlink\\message_definitions\\v1.0\\common.xml");
	common.open(QIODevice::ReadOnly);
	QByteArray commonbytes = common.readAll();
	common.close();
	QXmlStreamReader xml(commonbytes);
	while (!xml.atEnd())
	{
		if (xml.name() == "mavlink" && xml.isStartElement())
		{
			xml.readNext();
			while (xml.name() != "mavlink")
			{
				if (xml.name() == "messages" && xml.isStartElement())
				{
					xml.readNext();
					while (xml.name() != "messages")
					{
						if (xml.name() == "message" && xml.isStartElement())
						{
							QString messagename = xml.attributes().value("name").toString();
							QList<QPair<QString,QString> > fieldlist;
							xml.readNext();
							while (xml.name() != "message")
							{
								if (xml.name() == "description" && xml.isStartElement())
								{
									xml.readNext();
									QString description = xml.text().toString();
								}
								else if (xml.name() == "field" && xml.isStartElement())
								{
									QString type = xml.attributes().value("type").toString();
									QString name = xml.attributes().value("name").toString();
									fieldlist.append(QPair<QString,QString>(type,name));
									xml.readNext();
								}
								xml.readNext();
							}
							qSort(fieldlist.begin(),fieldlist.end(),MainWindow::compare);
							QString propertychunk = "";
							QString getchunk = "";
							QString setchunk = "";
							QString privatechunk = "";
							QString signalchunk = "";
							for (int i=0;i<fieldlist.size();i++)
							{
								QString camelCaseName = fieldlist.at(i).second;
								QString CamelCaseName = fieldlist.at(i).second;
								QString origname = fieldlist.at(i).second;
								if (origname.contains("_"))
								{
									QStringList split = origname.split("_");
									camelCaseName = split.at(0);
									CamelCaseName = split.at(0);
									CamelCaseName.replace(0,1,CamelCaseName.mid(0,1).toUpper());
									for (int j=1;j<split.size();j++)
									{
										QString tmp = split.at(j);
										camelCaseName += tmp.replace(0,1,tmp.mid(0,1).toUpper());
										CamelCaseName += tmp;
									}
								}
								else
								{
									CamelCaseName = CamelCaseName.replace(0,1,CamelCaseName.mid(0,1).toUpper());

								}

								QString type = "";
								if (fieldlist.at(i).first.contains("int64"))
								{
									type = "quint64";
								}
								else if (fieldlist.at(i).first.contains("uint"))
								{
									type = "unsigned int";
								}
								else if (fieldlist.at(i).first.contains("int"))
								{
									type = "int";
								}
								else if (fieldlist.at(i).first.contains("char["))
								{
									type = "QString";
								}
								else if (fieldlist.at(i).first.contains("float"))
								{
									type = "double";
								}
								else if (fieldlist.at(i).first.contains("double"))
								{
									type = "double";
								}
								propertychunk += "Q_PROPERTY(" + type + " " + fieldlist.at(i).second + " READ get" + CamelCaseName +" WRITE set" + CamelCaseName + " NOTIFY " + camelCaseName +"Changed)\r\n";
								getchunk += "    " + type + " get" + CamelCaseName + "() { return m_" + camelCaseName + "; }\r\n";
								setchunk += "    void set" + CamelCaseName + "(" + type + " " + camelCaseName + ") { if (m_" + camelCaseName + " != " + camelCaseName + ") { m_" + camelCaseName +" = " + camelCaseName + "; emit " +camelCaseName + "Changed(" + camelCaseName + "); } }\r\n";
								privatechunk += "    " + type + " m_" + camelCaseName + ";\r\n";
								signalchunk += "    void " + camelCaseName + "Changed(" + type + ");\r\n";
							}
							QString classheader = "";
							classheader += "class " + messagename + "\r\n";
							classheader += "{\r\n";
							classheader += propertychunk;
							classheader += "public:\r\n";
							classheader += getchunk;
							classheader += setchunk;
							classheader += "signals:\r\n";
							classheader += signalchunk;
							classheader += "private:\r\n";
							classheader += privatechunk;
							classheader += "};\r\n";
							ui->textBrowser->append(classheader + "\r\n\r\n");


							//Fields are now in proper order!
						}
						xml.readNext();
					}
				}
				xml.readNext();
			}
		}
		xml.readNext();
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}
