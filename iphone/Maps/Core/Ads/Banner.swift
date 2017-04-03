protocol Banner: MWMBanner {
  typealias EventName = String
  typealias ErrorDetails = [String : Any]
  typealias Success = (Banner) -> Void
  typealias Failure = (BannerType, EventName, ErrorDetails, NSError) -> Void
  typealias Click = (BannerType) -> Void
  func reload(success: @escaping Success, failure: @escaping Failure, click: @escaping Click)

  var isBannerOnScreen: Bool { get set }
  var isNeedToRetain: Bool { get }
  var isPossibleToReload: Bool { get }
  var type: BannerType { get }
}
