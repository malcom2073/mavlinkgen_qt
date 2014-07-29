#include <QFile>
#include <QXmlStreamReader>
#include <QPair>
#include <QDebug>
#include <QStringList>
#include <QString>
#include <QCoreApplication>
#include <QDir>

QMap<QString,int> lengths;

QList<QPair<QString,QString> > getArgs(int argc, char **argv)
{
	bool nextisarg = false;
	QString current = "";
	QString currentarg;
	QList<QPair<QString,QString> > retval;
	for (int i=1;i<argc;i++)
	{
		QString arg = QString(argv[i]);
		if (arg.startsWith("-") || arg.startsWith("--"))
		{
			if (nextisarg)
			{
				retval.append(QPair<QString,QString>(current,currentarg.trimmed()));
				currentarg = "";
			}
			nextisarg = true;
			current = arg;
		}
		else
		{
			if (nextisarg)
			{
				currentarg += arg + " ";
			}
			else
			{
				//Invalid
				qDebug() << "Invalid arg";
			}
		}
	}
	if (nextisarg)
	{
		//qDebug() << "Param with no arg" << current << currentarg;
		retval.append(QPair<QString,QString>(current,currentarg.trimmed()));
	}
	return retval;
}
void printHelp()
{
	qDebug() << "Help";
	qDebug() << "Available Commands:";
	qDebug() << "-h\t\t--help\t\tShow this help";
	qDebug() << "-i\t\t--input\t\tInput folder containing mavlink XML files";
	qDebug() << "-o\t\t--output\t\tOutput folder to create mavlink headers in";
}

bool compare(const QPair<QString,QString> &first, const QPair<QString,QString> &second)
{


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
		return lengths.value(firststr) > lengths.value(secondstr);
	}
	else
	{
		return false;
	}
	return false;
}


QPair<QString,QString> parseXml(QByteArray commonbytes,QString outputfolder)
{
	QPair<QString,QString> retval;
	QString mavlink_h_struct = "";
	QString mavlink_h_include = "HEADERS += \\";
	QXmlStreamReader xml(commonbytes);
	while (!xml.atEnd())
	{
		if (xml.name() == "mavlink" && xml.isStartElement())
		{
			QString mavlink_h_includes = "";

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
							int id = xml.attributes().value("id").toString().toInt();
							QString messagename = xml.attributes().value("name").toString();
							mavlink_h_struct += "\r\n" + messagename +"=" + QString::number(id) + ",";
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
							qStableSort(fieldlist.begin(),fieldlist.end(),compare);
							QString propertychunk = "";
							QString getchunk = "";
							QString setchunk = "";
							QString privatechunk = "";
							QString signalchunk = "";
							QString headers = "#include <QObject>\r\n#include <QString>\r\n#include <QByteArray>\r\n#include <stdint.h>\r\n";
							QString converter = "    void parseFromArray(QByteArray array)\r\n    {\r\n";
							int bufferpos = 0;
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
								int arraysize = -1;
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
								else if (fieldlist.at(i).first.contains("char"))
								{
									type = "unsigned char";
								}
								if (fieldlist.at(i).first.contains("char["))
								{
									QString charstr = fieldlist.at(i).first.mid(0,fieldlist.at(i).first.indexOf("["));
									QString fieldname = fieldlist.at(i).first;

									int charlen = fieldname.remove(charstr).remove("[").remove("]").toInt();
									converter += "        QString " + fieldlist.at(i).second + " = \"\";\r\n";
									converter += "        " + fieldlist.at(i).second + "=array.mid(" + QString::number(bufferpos) + "," + QString::number(charlen) +");\r\n";
									converter += "        this->m_" + camelCaseName + "=" + fieldlist.at(i).second + ";\r\n\r\n";
									bufferpos += charlen;

								}
								else if (fieldlist.at(i).first.contains("[") )
								{

									QString charstr = fieldlist.at(i).first.mid(0,fieldlist.at(i).first.indexOf("["));
									QString fieldname = fieldlist.at(i).first;
									int charlen = fieldname.remove(charstr).remove("[").remove("]").toInt();
									arraysize = charlen;
									int length = lengths[charstr];
									converter += "        this->m_" + camelCaseName + ".clear();\r\n";
									converter += "        for (int i=0;i<" + QString::number(charlen) + ";i++)\r\n";
									converter += "        {\r\n";
									converter += "              this->m_" + camelCaseName + ".append(*reinterpret_cast<" + charstr + "*>(array.data()+" + QString::number(bufferpos) + "));\r\n";
									converter += "        }\r\n";
									bufferpos+=length;
								}
								else
								{
									int length = lengths[fieldlist.at(i).first];
									if (length == 1)
									{
										converter += "        " + type + " " + fieldlist.at(i).second + "=0;\r\n";
										converter += "        " + fieldlist.at(i).second + " = static_cast<" + type + ">(array.at(" + QString::number(bufferpos) + "));\r\n";
										converter += "        this->m_" + camelCaseName + " = " + fieldlist.at(i).second + ";\r\n\r\n";
										bufferpos += 1;
									}
									else
									{
										converter += "        " + type + " " + fieldlist.at(i).second + "= 0;\r\n";
										converter += "        " + fieldlist.at(i).second + " = *reinterpret_cast<" + fieldlist.at(i).first + "*>(array.data()+" + QString::number(bufferpos) + ");\r\n";
										converter += "        this->m_" + camelCaseName + " = " + fieldlist.at(i).second + ";\r\n\r\n";
										bufferpos+=length;
									}

								}
								if (arraysize == -1)
								{
									getchunk += "    " + type + " get" + CamelCaseName + "() { return m_" + camelCaseName + "; }\r\n";
									setchunk += "    void set" + CamelCaseName + "(" + type + " " + camelCaseName + ") { m_" + camelCaseName +" = " + camelCaseName + "; }\r\n";
									privatechunk += "    " + type + " m_" + camelCaseName + ";\r\n";
									propertychunk += "Q_PROPERTY(" + type + " " + fieldlist.at(i).second + " READ get" + CamelCaseName +" WRITE set" + CamelCaseName + ")\r\n";
								}
								else
								{
									getchunk += "    QList<" + type + "> get" + CamelCaseName + "() { return m_" + camelCaseName + "; }\r\n";
									setchunk += "    void set" + CamelCaseName + "(QList<" + type + "> " + camelCaseName + ") { m_" + camelCaseName +" = " + camelCaseName + "; }\r\n";
									privatechunk += "    QList<" + type + "> m_" + camelCaseName + ";\r\n";
									propertychunk += "Q_PROPERTY(QList<" + type + "> " + fieldlist.at(i).second + " READ get" + CamelCaseName +" WRITE set" + CamelCaseName + ")\r\n";
								}

								signalchunk += "    void " + camelCaseName + "Changed(" + type + ");\r\n";
							}
							converter += "    }\r\n";
							QString classheader = headers;
							QString newclassname = "mavlink_message_" + messagename.toLower() + "_t";
							classheader += "class " + newclassname + " : public QObject\r\n";
							classheader += "{\r\n";
							classheader += "    Q_OBJECT\r\n";
							classheader += propertychunk;
							classheader += "public:\r\n";
							classheader += converter;
							classheader += getchunk;
							classheader += setchunk;
							classheader += "private:\r\n";
							classheader += privatechunk;
							classheader += "};\r\n";
							QFile outputfile(outputfolder + "/mavlink_message_" + messagename.toLower() + ".h");
							mavlink_h_include += "\r\n$$PWD/mavlink_message_" + messagename.toLower() + ".h \\";

							qDebug() << "Outputting to:" << outputfile.fileName();
							outputfile.open(QIODevice::ReadWrite | QIODevice::Truncate);
							outputfile.write(classheader.toAscii());
							outputfile.close();


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
	retval.first = mavlink_h_struct;
	retval.second = mavlink_h_include;
	return retval;
}



int main(int argc, char *argv[])
{
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

	QList<QPair<QString,QString> > args = getArgs(argc,argv);
	QString inputfile = "";
	QString outputdir = "";
	for (int i=0;i<args.size();i++)
	{
		if (args.at(i).first == "--input" || args.at(i).first == "-i")
		{
			inputfile = args.at(i).second;
		}
		else if (args.at(i).first == "--help" || args.at(i).first == "-h")
		{
			//print help here.
			printHelp();
			return 0;
		}
		else if (args.at(i).first == "--output" || args.at(i).first == "-o")
		{
			//Output file
			outputdir = args.at(i).second;
		}
		else
		{
			//Invalid arg
			qDebug() << "Unknown command: " + args.at(i).first;
			printHelp();
			return 0;
		}
	}
	if (inputfile == "" || outputdir == "")
	{
		//print help here
		qDebug() << "You must specify -i and -o to continue";
		printHelp();
		return 0;
	}
	QDir output(".");
	if (!QDir(outputdir).exists())
	{
		QDir(".").mkdir(outputdir);
	}
	output.cd(outputdir);

	QDir inputdir(inputfile);
	QString mavlink_enum_output = "enum mavlink_message_type\r\n{\r\n";
	QString headerlist ="";
	foreach(QString file,inputdir.entryList(QStringList() << "*.xml"))
	{
		qDebug() << "Found file:" << file;
		QFile common(inputdir.absoluteFilePath(file));
		common.open(QIODevice::ReadOnly);
		QByteArray commonbytes = common.readAll();
		common.close();
		QString outfile = file.mid(0,file.indexOf("."));
		if (!output.exists(outfile))
		{
			output.mkdir(outfile);
		}
		QPair<QString,QString> retval = parseXml(commonbytes,outputdir + "/" + outfile + "/");
		QString headers = retval.second.mid(0,retval.second.length()-1); //Get rid of the trailing slash
		QFile headerfile(outputdir + "/" + outfile + "/" + outfile + ".pri");
		headerfile.open(QIODevice::ReadWrite | QIODevice::Truncate);
		headerfile.write(headers.toAscii());
		headerfile.close();
		headerlist.append("include (" + outfile + "/" + outfile + ".pri)\r\n");

		mavlink_enum_output += retval.first;
	}
	mavlink_enum_output = mavlink_enum_output.mid(0,mavlink_enum_output.length()-1);
	mavlink_enum_output += "\r\n}\r\n";
	QFile mavlink_h(outputdir + "/mavlink.h");
	mavlink_h.open(QIODevice::ReadWrite | QIODevice::Truncate);
	//mavlink_h.write(headerlist.toAscii());
	mavlink_h.write(mavlink_enum_output.toAscii());
	mavlink_h.close();
	QFile mavlink_pri(outputdir + "/mavlink.pri");
	mavlink_pri.open(QIODevice::ReadWrite | QIODevice::Truncate);
	mavlink_pri.write(headerlist.toAscii());
	mavlink_pri.close();
	return 0;



	return 0;
}
