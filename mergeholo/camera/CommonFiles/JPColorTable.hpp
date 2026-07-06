#ifndef JP_COLOR_DEFINE_HPP
#define JP_COLOR_DEFINE_HPP
namespace JP
{
	class CColorTable
	{
	
	public:
	size_t size()
	{
		return gvctColorTable.size();
	}
	private:
		std::vector<std::tuple<QColor, QString, QString>> gvctColorTable = 
		{
			std::make_tuple( QColor(199, 237, 204),	"#C7EDCC", 	"豆沙绿		"), 	
			std::make_tuple( QColor(255, 255, 255),	"#FFFFFF", 	"银河白		"), 
			std::make_tuple( QColor(250, 249, 222),	"#FAF9DE", 	"杏仁黄		"), 
			std::make_tuple( QColor(255, 242, 226),	"#FFF2E2", 	"秋叶褐		"), 
			std::make_tuple( QColor(253, 230, 224),	"#FDE6E0", 	"胭脂红		"), 
			std::make_tuple( QColor(227, 237, 205),	"#E3EDCD", 	"青草绿		"), 
			std::make_tuple( QColor(220, 226, 241),	"#DCE2F1", 	"海天蓝		"), 
			std::make_tuple( QColor(233, 235, 254),	"#E9EBFE", 	"葛巾紫		"), 
			std::make_tuple( QColor(234, 234, 239),	"#EAEAEF", 	"极光灰		"), 
			std::make_tuple( QColor(183, 232, 189),	"#B7E8BD", 	"苹果绿		"), 
			std::make_tuple( QColor(204, 232, 207),	"#CCE8CF", 	"豆沙绿-略暗")
		};
	};
	
	
	
	
	
}
#endif // JP_COLOR_DEFINE_HPP
